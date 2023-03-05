#include <cmath>
#include <iostream>
#include "model.h"
#include "tgaimage.h"


const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model *model = NULL;
int *zbuffer = NULL;
Vec3f camera(0,0,3);

const int width = 800;
const int height = 800;
const int depth  = 255;


void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) { 
    bool steep = false; 
    if (std::abs(x0-x1)<std::abs(y0-y1)) { 
        std::swap(x0, y0); 
        std::swap(x1, y1); 
        steep = true; 
    } 
    if (x0>x1) { 
        std::swap(x0, x1); 
        std::swap(y0, y1); 
    } 
    int dx = x1-x0; 
    int dy = y1-y0; 
    int derror2 = std::abs(dy)*2; 
    int error2 = 0; 
    int y = y0; 
    for (int x=x0; x<=x1; x++) { 
        if (steep) { 
            image.set(y, x, color); 
        } else { 
            image.set(x, y, color); 
        } 
        error2 += derror2; 
        if (error2 > dx) { 
            y += (y1>y0?1:-1); 
            error2 -= dx*2; 
        } 
    } 
} 


int min(int a, int b, int c) {
    int res = 0;
    a <= b ? res = a : res = b;
    if(c < res) res = c;
    return res;
}

int max(int a, int b, int c) {
    int res = 0;
    a >= b ? res = a : res = b;
    if(c > res) res = c;
    return res;
}

//Fonction servant a calculer le rectangle couvrant d'un triangle donne par les points a, b et c
std::vector<int> trouver_rectangleCouvrant(std::vector<int> a, std::vector<int> b, std::vector<int> c) {
    int minX = min(a[0], b[0], c[0]);
    int maxX = max(a[0], b[0], c[0]);
    int minY = min(a[1], b[1], c[1]);
    int maxY = max(a[1], b[1], c[1]);

    return {minX, maxX, minY, maxY}; 
}

//Fonction servant a calculer l'aire d'un triangle donne par les points a, b et c
float calculerAire(std::vector<int> a, std::vector<int> b, std::vector<int> c) {
    std::vector<std::vector<int>> v = {a, b, c, a};
    float sum1 = 0;
    float sum2 = 0;

    int n = v.size();
    for(int i = 0; i < n-1; i++) {
        sum1 = sum1 + v[i][0] * v[i+1][1];
        sum2 = sum2 + v[i][1] * v[i+1][0];
    }
    sum1 = sum1 + v[n-1][0] * v[0][1];
    sum2 = sum2 + v[0][0] * v[n-1][1];
    float res = (sum1 - sum2) / 2;
    return res;
}

//Fonction servant a convertic une matrice en vecteur
Vec3f matrice_to_vecteur(Matrix m) {
    return Vec3f(m[0][0]/m[3][0], m[1][0]/m[3][0], m[2][0]/m[3][0]);
}

//Fonction servant a convertir un vecteur en matrice
Matrix vecteur_to_matrice(Vec3f v) {
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

Matrix viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x+w/2.f;
    m[1][3] = y+h/2.f;
    m[2][3] = depth/2.f;
    m[0][0] = w/2.f;
    m[1][1] = h/2.f;
    m[2][2] = depth/2.f;
    return m;
}

//Fonction servant a calculer le barycentre d'un point pour un P pour un triangle
//donne par les points a, b et c
Vec3f barycentre(std::vector<int> a, std::vector<int> b, std::vector<int> c, std::vector<int> p) {
    float aireABC = calculerAire(a,b,c);
    float alpha = calculerAire(p,b,c) / aireABC;
    float beta = calculerAire(p,c,a) / aireABC;
    float gamma = calculerAire(p,a,b) / aireABC;
    return Vec3f(alpha, beta, gamma);
} 

TGAColor getColor(TGAImage &t, Vec2i uv0, Vec2i uv1, Vec2i uv2, float alpha, float beta, bool deuxieme_moitie, float phi, bool swap) {
    Vec2i uvA = uv0 + (uv2-uv0)*alpha;
    Vec2i uvB;
    if(deuxieme_moitie) {
        uvB = uv1 + (uv2-uv1)*beta; 
    } else {
        uvB = uv0 + (uv1-uv0)*beta; 
    }
    if(swap) std::swap(uvA, uvB); 

    Vec2i uvP = uvA + (uvB-uvA)*phi;
    TGAColor color = t.get(uvP.x, uvP.y); 
    return color;           
}

void colorierTriangle(Vec3i sc0, Vec3i sc1, Vec3i sc2, int *zbuffer, TGAImage &image, TGAImage texture, int iface) {
    //std::vector<int> v = trouver_rectangleCouvrant(a,b,c);
    Vec2i uv0 = model->uv(iface, 0); 
    Vec2i uv1 = model->uv(iface, 1); 
    Vec2i uv2 = model->uv(iface, 2);  
    if (sc0.y==sc1.y && sc0.y==sc2.y) return; 
    if (sc0.y>sc1.y) { 
        std::swap(sc0, sc1); 
        std::swap(uv0, uv1); 
    }
    if (sc0.y>sc2.y) { 
        std::swap(sc0, sc2); 
        std::swap(uv0, uv2); 
    }
    if (sc1.y>sc2.y) { 
        std::swap(sc1, sc2); 
        std::swap(uv1, uv2); 
    }

    int hauteur = sc2.y-sc0.y;
    for (int i=0; i<hauteur; i++) {
        bool deuxieme_moitie = false;
        if(i > sc1.y-sc0.y || sc1.y==sc0.y) {
            deuxieme_moitie = true;
        } else {
            deuxieme_moitie = false;
        }
        int hauteur_segment = 0;
        if(deuxieme_moitie) {
            hauteur_segment = sc2.y-sc1.y;
        } else {
            hauteur_segment = sc1.y-sc0.y;
        }
        float alpha = (float)i/hauteur;
        float beta  = (float)(i-(deuxieme_moitie ? sc1.y-sc0.y : 0))/hauteur_segment; // be careful: with above conditions no division by zero here
        Vec3i A = sc0 + Vec3f(sc2-sc0)*alpha;
        Vec3i B = {0, 0, 0};
        if(deuxieme_moitie) {
            B = sc1 + Vec3f(sc2-sc1)*beta; 
        } else {
            B = sc0 + Vec3f(sc1-sc0)*beta; 
        }
        bool swap = false;
        if (A.x>B.x) { 
            std::swap(A, B); 
            swap = true; 
        }
        for(int j=A.x; j<=B.x; j++) {
            float phi = B.x==A.x ? 1. : (float)(j-A.x)/(float)(B.x-A.x);
            Vec3i P = Vec3f(A) + Vec3f(B-A)*phi;
            int idx = P.x+P.y*width;
            //Vec3f bc_screen = barycentre(a,b,c, {i,j});
            //if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            //TGAColor color = getColor(texture, bc_screen, iface); 
            //float Pz = 0;    
            //Pz += a[2]*bc_screen.x;
            //Pz += b[2]*bc_screen.y;
            //Pz += c[2]*bc_screen.z;
            if (zbuffer[idx] < P.z) {
                zbuffer[idx] = P.z;
                TGAColor color = getColor(texture, uv0, uv1, uv2, alpha, beta, deuxieme_moitie, phi, swap);
                image.set(P.x, P.y, color);  
            }
        }
    }
}

int main(int argc, char** argv) {
	
	model = new Model("./obj/african_head/african_head.obj");


	TGAImage image(800, 800, TGAImage::RGB);


    Vec3f light_dir(0,0,-1); // define light_dir

    TGAImage texture = TGAImage(); 
    texture.read_tga_file("obj/african_head_diffuse.tga"); 
    texture.flip_vertically(); 

    //zbuffer = new float[width*height];
   	//for(int i = 0; i < width*height;i++) 
    //zbuffer[i] = -10000;
    zbuffer = new int[width*height];
    for (int i=0; i<width*height; i++) {
        zbuffer[i] = std::numeric_limits<int>::min();
    }

    Matrix Projection = Matrix::identity(4);
    Matrix ViewPort = viewport(width/8, height/8, width*3/4, height*3/4);
    Projection[3][2] = -1.f/camera.z;
    
	for(int i = 0; i < model->nfaces();i++) {
		std::vector<int> v = model->face(i);
        Vec3f v0 = model->vert(v[0]);
		Vec3f v1 = model->vert(v[1]);
        Vec3f v2 = model->vert(v[2]);
        Vec3f world_coords[3];
        world_coords[0] = v0;
        world_coords[1] = v1;
        world_coords[2] = v2;
        Vec3f screen_coords[3];
        screen_coords[0] = matrice_to_vecteur(ViewPort*Projection*vecteur_to_matrice(v0));
        screen_coords[1] = matrice_to_vecteur(ViewPort*Projection*vecteur_to_matrice(v1));
        screen_coords[2] = matrice_to_vecteur(ViewPort*Projection*vecteur_to_matrice(v2));
        //int x0 = (v0.x + 1.) * width/2. +.5;
		//int y0 = (v0.y + 1.) * height/2. +.5;
        //int z0 = (v0.z + 1.); 
		//int x1 = (v1.x + 1.) * width/2. +.5;
		//int y1 = (v1.y + 1.) * height/2. +.5;
        //int z1 = (v1.z + 1.); 
        //int x2 = (v2.x + 1.) * width/2. +.5;
		//int y2 = (v2.y + 1.) * height/2. +.5;
        //int z2 = (v2.z + 1.); 

        Vec3f n = (world_coords[2]-world_coords[0])^(world_coords[1]-world_coords[0]);
        n.normalize();
        float intensity = n*light_dir;
		// for(int j = 0; j < 3; j++) {
		// 	Vec3f v0 = model->vert(v[j]);
		// 	Vec3f v1 = model->vert(v[(j+1)%3]);
		// 	int x0 = (v0.x + 1.) * width/2.;
		// 	int y0 = (v0.y + 1.) * height/2.;
		// 	int x1 = (v1.x + 1.) * width/2.;
		// 	int y1 = (v1.y + 1.) * height/2.;
		// 	line(x0,y0,x1,y1, image, white);
		// }
        if(intensity>0) {
            colorierTriangle(screen_coords[0], screen_coords[1], screen_coords[2], zbuffer, image, texture, i);
        }
	}

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
    delete model;
    delete [] zbuffer;
	return 0;
}

//TEXTURING : 
//3D : triangle = {v1=x1y1z1, v2=x2y2z2, v3=x3y3z3}s
// -> 2D : triangle = {vsc1=x1y1, vsc2=x2y2, vt3=x3y3} (t pour texture)
//Dans .obj : f a1b1c1 a2b2c2 a3b3c3
//            a1 a2 a3 -> indice dans nuage "v_" +1
//            b1 b2 b3 -> indice dans nuage "vt_ " +1s
//P = alpha*A + beta*B + teta*C
//(u,v) = alpha*(u1,v1) + beta*(u2,v2) + teta*(u3,v3)

