#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

// App structure
typedef struct
{
	SDL_Window *window;
	SDL_GLContext *gl_context;
} app_t;

// Initialize app
app_t *app_init ()
{
	static app_t app;

	SDL_Init (SDL_INIT_VIDEO);

	app.window = 
		SDL_CreateWindow(
			"c-fractals",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			900, 900,
			SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

	SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	
	app.gl_context = SDL_GL_CreateContext (app.window);

	return &app;
}

// Exit app
int app_exit (app_t *app)
{
  	SDL_GL_DeleteContext (app->gl_context);  
  	SDL_DestroyWindow (app->window);
  	SDL_Quit ();
  	return 0;
}

// Load file
static void load_file (const char *fileName, Uint8 *out_buf, int size)
{
  	SDL_RWops *rw = SDL_RWFromFile (fileName, "rb");
  	if (rw == NULL)
  	{
  		printf ("error\n");
  		return;
  	}
  	SDL_RWread (rw, out_buf, size, 1);
	SDL_RWclose (rw);
}

const float pi = 3.14159265359f;

typedef struct
{
    float x;
    float y;
} vec2_t;

typedef struct
{
    vec2_t x;
    vec2_t y;
} drawLine_t;

typedef struct
{
    vec2_t x;
    vec2_t y;
    float degrees;
    float length;
} line_t;

typedef struct
{
    line_t x;
    line_t y;
} branch_t;

typedef struct
{
    float leftAngleFactor;
    float rightAngleFactor;
    float leftScaleFactor;
    float rightScaleFactor;
} fractalInfo_t;

const float torad = 0.0174532925f;

vec2_t make_endpoint (float degrees, float length, vec2_t v)
{
    vec2_t v1;

    v1.x = v.x + length * cos (degrees * torad);
    v1.y = v.y + length * sin (degrees * torad);

    return v1;
}

line_t make_line (float degrees, float length, line_t line)
{
    line_t line1;

    line1.x = line.y;
    line1.y = make_endpoint (degrees, length, line.y);
    line1.degrees = degrees;
    line1.length = length;

    return line1;
}

branch_t make_branch (line_t line, fractalInfo_t finfo)
{
    branch_t branch;

    branch.x = make_line (line.degrees - finfo.leftAngleFactor, line.length * finfo.leftScaleFactor, line);
	branch.y = make_line (line.degrees + finfo.rightAngleFactor, line.length * finfo.rightScaleFactor, line);

	return branch;
}	

void add_branch (branch_t branch, int index, line_t *lines)
{
	*((branch_t *)&lines[index]) = branch;
}

void generate_lines (int count, line_t *lines, fractalInfo_t finfo)
{
	int totalLineCount = 2;
	int prevTotalLineCount = 0;

	for (int n = 0; n < count; ++n)
	{
		const int i = totalLineCount;
		const int j = prevTotalLineCount;
		const int c = totalLineCount - prevTotalLineCount;

		for (int k = 0; k < c; ++k)
		{
			int prevIndex = j + k + 1;
			int index = i + (k * 2) + 1;

			branch_t branch = make_branch (lines[prevIndex], finfo);
			add_branch (branch, index, lines);
		}

		prevTotalLineCount = i;
		totalLineCount = i + (c * 2);
	}
}

void make_lines (int count, line_t *lines, fractalInfo_t finfo)
{
  	line_t firstLine;
  	firstLine.x.x = 0.0f;
  	firstLine.x.y = -1.0f;
  	firstLine.y.x = 0.0f;
  	firstLine.y.y = -0.5f;
   	firstLine.degrees = 90.0f;
  	firstLine.length = 0.5f;

  	lines[0] = firstLine;
  	*((branch_t *)&lines[1]) = make_branch (lines[0], finfo);

  	generate_lines (count, lines, finfo);
}

#define MAX_LINES 65536
#define SHADER_FILE_SIZE 4096

// Start app loop
void app_loop (app_t *app)
{
  	SDL_Event e;

  	drawLine_t zeroDrawLine;
  	zeroDrawLine.x.x = 0.0f;
  	zeroDrawLine.x.y = 0.0f;
  	zeroDrawLine.y.x = 0.0f;
  	zeroDrawLine.y.y = 0.0f;
  	drawLine_t vertices[MAX_LINES] = { zeroDrawLine };

  	line_t zeroLine;
  	zeroLine.x.x = 0.0f;
  	zeroLine.x.y = 0.0f;
  	zeroLine.y.x = 0.0f;
  	zeroLine.y.y = 0.0f;
  	zeroLine.degrees = 0.0f;
  	zeroLine.length = 0.0f;
  	line_t lines[MAX_LINES] = { zeroLine }; 

	/******************************************************/

  	GLuint vbo;
	glGenBuffers (1, &vbo);

	glBindBuffer (GL_ARRAY_BUFFER, vbo);

	glBufferData (GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	/******************************************************/

	Uint8 vertexSource[SHADER_FILE_SIZE] = { '\0' };
	Uint8 fragmentSource[SHADER_FILE_SIZE] = { '\0' };

	load_file ("v.vertex", vertexSource, SHADER_FILE_SIZE);
	const GLchar *testVertex = (GLchar*)vertexSource;

	load_file ("f.fragment", fragmentSource, SHADER_FILE_SIZE);
	const GLchar *testFragment = (GLchar*)fragmentSource;

	GLuint vertexShader = glCreateShader (GL_VERTEX_SHADER);
	glShaderSource (vertexShader, 1, &testVertex, NULL);	
	glCompileShader (vertexShader);

	GLuint fragmentShader = glCreateShader (GL_FRAGMENT_SHADER);
	glShaderSource (fragmentShader, 1, &testFragment, NULL);
	glCompileShader (fragmentShader);

	/******************************************************/

	GLuint shaderProgram = glCreateProgram ();
	glAttachShader (shaderProgram, vertexShader);
	glAttachShader (shaderProgram, fragmentShader);

	glBindFragDataLocation (shaderProgram, 0, "color");

	glLinkProgram (shaderProgram);

	glUseProgram (shaderProgram);

	/******************************************************/

	GLuint vao;
	glGenVertexArrays (1, &vao);

	glBindVertexArray (vao);

	GLint posAttrib = glGetAttribLocation (shaderProgram, "position");

	glVertexAttribPointer (posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray (posAttrib);

	GLint uni_color = glGetUniformLocation (shaderProgram, "uni_color");

	/******************************************************/
	
  	fractalInfo_t finfo;
  	finfo.leftAngleFactor = 20.0f;
  	finfo.rightAngleFactor = 20.0f;
  	finfo.leftScaleFactor = 0.7f;
  	finfo.rightScaleFactor = 0.7f;
  
  	while (e.type != SDL_QUIT) {
		SDL_PollEvent (&e);

	   	glClear (GL_COLOR_BUFFER_BIT);
	
		/******************************************************/

		if (e.type == SDL_KEYDOWN) {
			if (e.key.keysym.sym == SDLK_LEFT) {
				finfo.rightAngleFactor += 0.5f;
			}

			if (e.key.keysym.sym == SDLK_RIGHT) {
				finfo.rightAngleFactor -= 0.5f;
			}

			if (e.key.keysym.sym == SDLK_UP) {
				finfo.leftAngleFactor += 0.5f;
			}
			
			if (e.key.keysym.sym == SDLK_DOWN) {
				finfo.leftAngleFactor -= 0.5f;
			}
			
			if (e.key.keysym.sym == SDLK_1) {
				finfo.leftScaleFactor += 0.01f;
			}
			
			if (e.key.keysym.sym == SDLK_2) {
				finfo.leftScaleFactor -= 0.01f;
			}
			
			if (e.key.keysym.sym == SDLK_3) {
				finfo.rightScaleFactor += 0.01f;
			}
			
			if (e.key.keysym.sym == SDLK_4) {
				finfo.rightScaleFactor -= 0.01f;
			}
		}

		make_lines (14, lines, finfo);

		// map
		for (int i = 0; i < MAX_LINES; ++i)
		{
			vertices[i].x = lines[i].x;
			vertices[i].y = lines[i].y;
		}

		float rfactor = finfo.rightAngleFactor * torad;
		float lfactor = finfo.leftAngleFactor * torad;
		glUniform4f (uni_color, sin (rfactor), sin (lfactor), cos (rfactor), 1.0f);

		glBindBuffer (GL_ARRAY_BUFFER, vbo);

		glBufferData (GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

		/******************************************************/

		glDrawArrays (GL_LINES, 0, MAX_LINES * 2);
	    
		SDL_GL_SwapWindow (app->window);
  	}
}

// Main
int main (int argc, char *argv[])
{  
	app_t *app = app_init ();

	app_loop (app);

	return app_exit (app);
}
