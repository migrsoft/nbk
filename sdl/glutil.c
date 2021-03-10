/*
 * glutil.c
 *
 *  Created on: 2012-4-29
 *      Author: wuyulun
 */

#include <assert.h>
#include "glutil.h"

//#define USE_ES_V1
#define USE_ES_V2

// ----------------------------------------------------------------------------
// Use OpenGL ES v1
// ----------------------------------------------------------------------------

#ifdef USE_ES_V1

static SDL_Surface *TextureSurface = NULL;
static Uint32 TextureId;
static GLuint VBOId[2];

SDL_Surface* gl_init(void)
{
    SDL_Surface *dev = NULL;  // Temporay Surface will be deallocated from SDL_Quit

    GLshort screen[] = {0,400,0, 320,400,0, 320,0,0, 0,0,0};//Vertex Coordinates
    GLbyte  texture[] = {0,1, 1,1, 1,0, 0,0};               //Texture Coordinates being used for VBO

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1); //We are using OpenGL ES 1.1
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);          //Turning on Double Buffering
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);    //Making sure SDL picks one with Hardware Acceleration

    dev = SDL_SetVideoMode(0, 0, 0, SDL_OPENGL);

    screen[1] = dev->h;
    screen[3] = dev->w;
    screen[4] = dev->h;
    screen[6] = dev->w;

    TextureSurface = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_ANYFORMAT, dev->w, dev->h, dev->format->BitsPerPixel,
        dev->format->Rmask, dev->format->Gmask, dev->format->Bmask, dev->format->Amask);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);    //OpenGL ES Stuff here you should know what is going on
    glViewport(0, 0, dev->w, dev->h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(0, dev->w, dev->h, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glGenTextures(1, &TextureId);
    glBindTexture(GL_TEXTURE_2D, TextureId);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //Setting up VBO here
    glGenBuffers(2, VBOId);

    glBindBuffer(GL_ARRAY_BUFFER, VBOId[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLshort) * 12, screen, GL_STATIC_DRAW); 

    glBindBuffer(GL_ARRAY_BUFFER, VBOId[1]);
    glBufferData(GL_ARRAY_BUFFER, 8, texture, GL_STATIC_DRAW);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //tried different Values no big change in Performance from Driver

    //glTexImage2D will cause a GLError of Invalid Value just ignore it 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dev->w, dev->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    //Here we are setting up the Vertex Array and Texture Array for VBO so there is no need to call it 
    //later on during the Loop
    glBindBuffer(GL_ARRAY_BUFFER, VBOId[0]);
    glVertexPointer(3, GL_SHORT, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, VBOId[1]);
    glTexCoordPointer(2, GL_BYTE, 0, 0);             

    return TextureSurface; //we are returning the Texture Surface 
}

void gl_end(void)
{
    glDeleteTextures(1, &TextureId);
    glDeleteBuffers(2, VBOId);
    SDL_FreeSurface(TextureSurface);
}

void gl_drawSurfaceAsTexture(SDL_Surface* s)
{
    //Using glTexSubImage2D since we are using the same Texture there is no Performance differences between
    //glTexSubImage2D and glTexImage2D

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TextureSurface->w, TextureSurface->h, GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);

    //This is it and it will draw our Texture for us since OpenGL ES allready got all the Vertex and Texture Coordinates  
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glFinish(); //We can not call glFlush() since SDL_GL_SwapBuffers allready will do it for us,
    //but we have to call glFinish otherwise the will be a big stall on SDL_GL_SwapBuffers
    //and the screen will update only 6Fps

    SDL_GL_SwapBuffers();
}

#endif // USE_ES_V1

// ----------------------------------------------------------------------------
// Use OpenGL ES v2
// ----------------------------------------------------------------------------

#ifdef USE_ES_V2

static GLuint programObject;
static GLint  positionLoc;
static GLint  texCoordLoc;
static GLint samplerLoc;
static GLuint texture = 0;

static SDL_Surface* surface = NULL;

static float portrait_vertexCoords[] = { -1, 1, -1, -1, 1, 1, 1, -1 };
static float texCoords[] = { 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0 };
static GLushort indices[] = { 0, 1, 2, 1, 2, 3 };

static GLuint esLoadShader(GLenum type, const char *shaderSrc)
{
	GLuint shader;
	GLint compiled;

	// Create the shader object
	shader = glCreateShader(type);

	if (shader == 0)
		return 0;

	// Load the shader source
	glShaderSource(shader, 1, &shaderSrc, NULL);

	// Compile the shader
	glCompileShader(shader);

	// Check the compile status
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled) {
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static GLuint esLoadProgram(const char *vertShaderSrc, const char *fragShaderSrc)
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;

	// Load the vertex/fragment shaders
	vertexShader = esLoadShader(GL_VERTEX_SHADER, vertShaderSrc);
	if (vertexShader == 0)
		return 0;

	fragmentShader = esLoadShader(GL_FRAGMENT_SHADER, fragShaderSrc);
	if (fragmentShader == 0) {
		glDeleteShader(vertexShader);
		return 0;
	}

	// Create the program object
	programObject = glCreateProgram();

	if (programObject == 0)
		return 0;

	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);

	// Link the program
	glLinkProgram(programObject);

	// Check the link status
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

	if (!linked) {
		glDeleteProgram(programObject);
		return 0;
	}

	// Free up no longer needed shader resources
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return programObject;
}

SDL_Surface* gl_init()
{
    GLbyte vShaderStr[] =
        "attribute vec4 a_position;   \n"
        "attribute vec2 a_texCoord;   \n"
        "varying vec2 v_texCoord;     \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_Position = a_position; \n"
        "   v_texCoord = a_texCoord;  \n"
        "}                            \n";

    GLbyte fShaderStr[] =
        "precision mediump float;                            \n"
        "varying vec2 v_texCoord;                            \n"
        "uniform sampler2D s_texture;                        \n"
        "void main()                                         \n"
        "{                                                   \n"
        "  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
        "}                                                   \n";

    assert(!SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8));
	assert(!SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8));
	assert(!SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8));
	assert(!SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8));
	assert(!SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1));
	assert(!SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1));
	assert(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2));

    surface = SDL_SetVideoMode(0, 0, 0, SDL_OPENGL);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Remove unnecessary operations..
    glDepthFunc(GL_ALWAYS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);

    //Enable alpha blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	// Load the shaders and get a linked program object
	programObject = esLoadProgram((char*)vShaderStr, (char*)fShaderStr);
	assert(programObject != 0);

	// Get the attribute locations
	positionLoc = glGetAttribLocation(programObject, "a_position");
	texCoordLoc = glGetAttribLocation(programObject, "a_texCoord");

	// Get the sampler location
	samplerLoc = glGetUniformLocation(programObject, "s_texture");

	glGenTextures(1, &texture);

	return surface;
}

void gl_end(void)
{
	glDeleteTextures(1, &texture);
	glDeleteProgram(programObject);
}

void gl_drawSurfaceAsTexture(SDL_Surface* s)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glUseProgram(programObject);

	glVertexAttribPointer(positionLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), portrait_vertexCoords);
	glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), texCoords);

	glEnableVertexAttribArray(positionLoc);
	glEnableVertexAttribArray(texCoordLoc);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glUniform1i(samplerLoc, 0);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

	SDL_GL_SwapBuffers();
}

#endif // USE_ES_V2
