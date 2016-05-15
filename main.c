#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <stdbool.h>

// Screen dimension constants
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 768;

// Main loop flag
bool quit = false;

// Starts up SDL, creates window, and initializes OpenGL
bool init();

// Initializes matrices and clear color
bool initGL();

// Input handler
void handleKeys(unsigned char key, int x, int y);

// Per frame update
void update();

// Renders quad to the screen
void render();

// Frees media and shuts down SDL
void myclose();

// The window we'll be rendering to
SDL_Window* gWindow = NULL;

// OpenGL context
SDL_GLContext gContext;

// Render flag
bool gRenderQuad = true;

bool init() {
    // Initialization flag
    bool success = true;

    // Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        success = false;
    } else {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

        // Create window
        gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        if(gWindow == NULL) {
            printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
            success = false;
        } else {
            // Create context
            gContext = SDL_GL_CreateContext(gWindow);
            if(gContext == NULL) {
                printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
                success = false;
            } else {
                // Use Vsync
                if(SDL_GL_SetSwapInterval(1) < 0) {
                    printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
                }

                // Initialize OpenGL
                if(!initGL()) {
                    printf("Unable to initialize OpenGL!\n");
                    success = false;
                }
            }
        }
    }

    return success;
}

bool initGL()
{
    bool success = true;
    GLenum error = GL_NO_ERROR;

    // Initialize Projection Matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Check for error
    error = glGetError();
    if(error != GL_NO_ERROR) {
        success = false;
    }

    // Initialize Modelview Matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Check for error
    error = glGetError();
    if(error != GL_NO_ERROR) {
        success = false;
    }
    
    // Initialize clear color
    glClearColor(0.f, 0.f, 0.f, 1.f);
    
    // Check for error
    error = glGetError();
    if(error != GL_NO_ERROR) {
        success = false;
    }

    return success;
}

void handleKeys(unsigned char key, int x, int y) {
    if(key == 'q') {
        quit = true;
    }
}

void update() {
}

void render()
{
    // Clear color buffer
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render quad
    if(gRenderQuad) {
        glRotatef(0.4f,0.0f,1.0f,0.0f);    // Rotate The cube around the Y axis
        glRotatef(0.2f,1.0f,1.0f,1.0f);
        glColor3f(0.0f,1.0f,0.0f); 

        glBegin(GL_QUADS);
            glVertex2f(-0.5f, -0.5f);
            glVertex2f(0.5f, -0.5f);
            glVertex2f(0.5f, 0.5f);
            glVertex2f(-0.5f, 0.5f);
        glEnd();
    }
}

void myclose() {
    // Destroy window    
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;

    // Quit SDL subsystems
    SDL_Quit();
}

int main(int argc, char* args[])
{
    // Start up SDL and create window
    if(!init()) {
        printf( "Failed to initialize!\n" );
    } else {
        // Event handler
        SDL_Event e;
        
        // Enable text input
        SDL_StartTextInput();

        // While application is running
        while(!quit) {
            // Handle events on queue
            while(SDL_PollEvent(&e) != 0) {
                // User requests quit
                if(e.type == SDL_QUIT) {
                    quit = true;
                } else if( e.type == SDL_TEXTINPUT ) { // Handle keypress with current mouse position
                    int x = 0, y = 0;
                    SDL_GetMouseState(&x, &y);
                    handleKeys(e.text.text[ 0 ], x, y);
                }
            }

            // Render quad
            render();
            
            // Update screen
            SDL_GL_SwapWindow(gWindow);
        }

        // Disable text input
        SDL_StopTextInput();
    }

    // Free resources and close SDL
    myclose();

    return 0;
}

