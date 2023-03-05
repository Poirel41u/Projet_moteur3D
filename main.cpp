#include <cmath>
#include <iostream>
#include "model.h"
#include "tgaimage.h"


const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model *model = NULL;

const int width = 800;
const int height = 800;

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

std::vector<int> trouver_rectangleCouvrant(std::vector<int> a, std::vector<int> b, std::vector<int> c) {
    int minX = min(a[0], b[0], c[0]);
    int maxX = max(a[0], b[0], c[0]);
    int minY = min(a[1], b[1], c[1]);
    int maxY = max(a[1], b[1], c[1]);

    return {minX, maxX, minY, maxY}; 
}


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

Vec3f barycentre(std::vector<int> a, std::vector<int> b, std::vector<int> c, std::vector<int> p) {
    float aireABC = calculerAire(a,b,c);
    float alpha = calculerAire(p,b,c) / aireABC;
    float beta = calculerAire(p,c,a) / aireABC;
    float gamma = calculerAire(p,a,b) / aireABC;
    return Vec3f(alpha, beta, gamma);
} 

void colorierTriangle(std::vector<int> a, std::vector<int> b, std::vector<int> c, float *zbuffer, TGAImage &image, TGAColor color) {
    std::vector<int> v = trouver_rectangleCouvrant(a,b,c);
    // de i = v.minX à maxX
    //std::cout << "\n a[2]";
    //std::cout << a[2];
    // std::cout << "\n b[2]";
    // std::cout << b[2];
    // std::cout << "\n c[2]";
    // std::cout << c[2];
    for(int i = v[0]; i < v[1]; i++) {
        // de j = v.minY à maxY
        for(int j = v[2]; j < v[3]; j++) {
            Vec3f bc_screen = barycentre(a,b,c, {i,j});
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            float Pz = 0;    
            Pz += a[2]*bc_screen.x;
            Pz += b[2]*bc_screen.y;
            Pz += c[2]*bc_screen.z;
            //std::cout << "bc_screen : ";
            //std::cout << bc_screen ;
            if (zbuffer[int(i+j*width)] < Pz) {
                zbuffer[int(i+j*width)] = Pz;
                image.set(i, j, color);  
            }
        }
    }
}

int main(int argc, char** argv) {
	
	model = new Model("./obj/african_head/african_head.obj");

	TGAImage image(800, 800, TGAImage::RGB);

    Vec3f light_dir(0,0,-1); // define light_dir

    float *zbuffer = new float[width*height];
   	for(int i = 0; i < width*height;i++) 
    zbuffer[i] = -10000;
    
	for(int i = 0; i < model->nfaces();i++) {
		std::vector<int> v = model->face(i);
        Vec3f v0 = model->vert(v[0]);
		Vec3f v1 = model->vert(v[1]);
        Vec3f v2 = model->vert(v[2]);
        Vec3f world_coords[3];
        world_coords[0] = v0;
        world_coords[1] = v1;
        world_coords[2] = v2;
        int x0 = (v0.x + 1.) * width/2. +.5;
		int y0 = (v0.y + 1.) * height/2. +.5;
        int z0 = (v0.z + 1.); 
		int x1 = (v1.x + 1.) * width/2. +.5;
		int y1 = (v1.y + 1.) * height/2. +.5;
        int z1 = (v1.z + 1.); 
        int x2 = (v2.x + 1.) * width/2. +.5;
		int y2 = (v2.y + 1.) * height/2. +.5;
        int z2 = (v2.z + 1.); 

        Vec3f n = (world_coords[2]-world_coords[0])^(world_coords[1]-world_coords[0]);
        n.normalize();
        float intensity = n*light_dir;
        //std::cout << "intensity : ";
        //std::cout << intensity;
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

            colorierTriangle({x0, y0, z0}, {x1, y1, z1}, {x2, y2, z2}, zbuffer, image, TGAColor(255*intensity, 255*intensity, 255*intensity, 255));
        }
	}

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	return 0;
}

//TEXTURING : 
//3D : triangle = {v1=x1y1z1, v2=x2y2z2, v3=x3y3z3}s
// -> 2D : triangle = {vt1=x1y1, vt2=x2y2, vt3=x3y3} (t pour texture)
//Dans .obj : f a1b1c1 a2b2c2 a3b3c3
//            a1 a2 a3 -> indice dans nuage "v_" +1
//            b1 b2 b3 -> indice dans nuage "vt_ " +1s
//P = alpha*A + beta*B + teta*C
//(u,v) = alpha*(u1,v1) + beta*(u2,v2) + teta*(u3,v3)

