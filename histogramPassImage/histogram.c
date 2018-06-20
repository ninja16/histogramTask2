#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "struct.h"

#define RGB_COMPONENT_COLOR 255

static PPMImage *readPPM() {
	char buff[16];
	PPMImage *img;
	FILE *fp;
	int c, rgb_comp_color;
  	fp = stdin;

	if (!fgets(buff, sizeof(buff), fp)) {
		perror("stdin");
		exit(1);
	}

	if (buff[0] != 'P' || buff[1] != '6') {
		fprintf(stderr, "Invalid image format (must be 'P6')\n");
		exit(1);
	}

	img = (PPMImage *) malloc(sizeof(PPMImage));
	if (!img) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	c = getc(fp);
	while (c == '#') {
		while (getc(fp) != '\n')
			;
		c = getc(fp);
	}

	ungetc(c, fp);
	if (fscanf(fp, "%d %d", &img->x, &img->y) != 2) {
		fprintf(stderr, "Invalid image size (error loading)\n");
		exit(1);
	}

	if (fscanf(fp, "%d", &rgb_comp_color) != 1) {
		fprintf(stderr, "Invalid rgb component (error loading)\n");
		exit(1);
	}

	if (rgb_comp_color != RGB_COMPONENT_COLOR) {
		fprintf(stderr, "Image does not have 8-bits components\n");
		exit(1);
	}

	while (fgetc(fp) != '\n')
		;
	img->data = (PPMPixel*) malloc(img->x * img->y * sizeof(PPMPixel));

	if (!img) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	if (fread(img->data, 3 * img->x, img->y, fp) != img->y) {
		fprintf(stderr, "Error loading image.\n");
		exit(1);
	}

	return img;
}


void Histogram(PPMImage *image, float *h) {

	pid_t pid;

	int i, j,  k, l, x, count;
	int rows, cols;

	int fd[2];
	char fdChar[12];
	char* args[3];
	char* envp[] = {NULL};
	
	float n = image->y * image->x;
	float childN;

	PPMImage *buf = (PPMImage *)malloc(sizeof(PPMImage));
	buf->data = (PPMPixel*)malloc(sizeof(PPMPixel)*n);

	cols = image->x;
	rows = image->y;

	if(pipe(fd)==-1)
	{
		perror("Error creating pipe\n");
		exit(1);
	}

	for (i = 0; i < n; i++) {
		image->data[i].red = floor((image->data[i].red * 4) / 256);
		image->data[i].blue = floor((image->data[i].blue * 4) / 256);
		image->data[i].green = floor((image->data[i].green * 4) / 256);
	}

	count = 0;
	x = 0;
	
	if((pid=fork())<0)
	{
		perror("error forking child.\n");
		exit(1);
	}

	if(pid > 0)
	{
		printf("Parent begins\n");
		printf("n = %f\n",n);
		close(fd[0]);
		write(fd[1],&n,sizeof(float));
		write(fd[1],image,sizeof(PPMImage));
		write(fd[1],image->data,(sizeof(PPMPixel)*n));
		close(fd[1]);
		wait(&pid);
		printf("Parent ends\n");
	}

	else
	{
		close(fd[1]);
		/**/
		snprintf(fdChar,12,"%i",fd[0]); // A 32 bit number can't take more than 11 characters, + a terminating 0

		args[0] = "child";
		args[1] = fdChar;
		args[2] = (char*)0;

		execve("child",args,envp);

		perror("error execve\n");
		exit(1);

		
		/**/
		/*
		read(fd[0],&childN,sizeof(float));
		read(fd[0],buf,sizeof(PPMImage));
		read(fd[0],buf->data,sizeof(PPMPixel)*n);
		
		for(j = 0; j <= 3; j++){
			for(k = 0; k <= 3; k++){
				for(l = 0; l <= 3; l++){
					for(i = 0; i < childN; i++){
						if(buf->data[i].red == j && buf->data[i].green == k && buf->data[i].blue == l){
							count++;
						}
					}
					h[x] = count / n;
					count = 0;
					x++;
				}
			}
		}

		for(i = 0; i < 64; i++){
			printf("%d = %0.3f ",i,h[i]);
		}
		printf("\n");
		*/
	}
}

int main(int argc, char *argv[]) {

	int i;

	PPMImage *image = readPPM();

	float *h = (float*)malloc(sizeof(float) * 64);

	for(i=0; i < 64; i++) h[i] = 0.0;

	Histogram(image, h);

	return 0;
}
