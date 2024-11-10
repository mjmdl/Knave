#include <assert.h>
#include <time.h>

#include "knave.h"
#define KGL_HOST
#include "knave_opengl.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

static Ku64 klin_get_nanosecond(void)
{
	struct timespec timespec;
	assert(clock_gettime(CLOCK_MONOTONIC, &timespec) == 0);
	return (timespec.tv_sec * K_NANOSECONDS_PER_SECOND) + timespec.tv_nsec;
}

static void klin_print_opengl_error(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *user_param)
{
	KERROR("OpenGL error #%d: source = %d, type = %d, severity = %d: %.*s", id, source, type, severity, (int)length, message);
}

static int klin_print_xlib_error(Display *display, XErrorEvent *event)
{
	char description[512];
	XGetErrorText(display, event->error_code, description, sizeof(description));
	KERROR("X11 error #%d: %.*s", event->error_code, (int)sizeof(description), description);
	return 0;
}

int main(int argc, char *argv[])
{
	XSetErrorHandler(klin_print_xlib_error);
	
	typedef void(*Kgl_Debug_Message_Callback)(Kgl_Debug_Callback callback, const void *user_param);
	Kgl_Debug_Message_Callback glDebugMessageCallback = (Kgl_Debug_Message_Callback)glXGetProcAddressARB((const GLubyte *)"glDebugMessageCallback");
	if (glDebugMessageCallback != NULL)
	{
		glDebugMessageCallback(klin_print_opengl_error, NULL);
	}
	else
	{
		KWARN("Could not set the OpenGL debug callback.");
	}

	int opengl_functions_loaded_count = 0;
#define X(RETURN, FUNCTION, ARGUMENTS) \
	FUNCTION = (K_FN(FUNCTION))glXGetProcAddressARB((const GLubyte *)#FUNCTION); \
	if (FUNCTION == NULL) \
	{ \
		KFATAL("Could not load the OpenGL function: " #FUNCTION "."); \
		return 1; \
	} \
	else \
	{ \
		KDEBUG("Loaded OpenGL function " #FUNCTION "."); \
		opengl_functions_loaded_count++; \
	}
	KGL_FUNCTIONS
#undef X
	KTRACE("%d OpenGL functions loaded.", opengl_functions_loaded_count);
	
	Display *display = XOpenDisplay(NULL);
	if (display == NULL)
	{
		KFATAL("Could not open the X11 display.");
		return 1;
	}
	else
	{
		KDEBUG("X11 display opened: %p", (void *)display);
	}

	int major_glx = 0;
	int minor_glx = 0;
	if (glXQueryVersion(display, &major_glx, &minor_glx) == True)
	{
		const int Minimum_Major_Glx = 1;
		const int Minimum_Minor_Glx = 3;
		if ((major_glx < Minimum_Major_Glx) || ((minor_glx < Minimum_Minor_Glx) && (major_glx == Minimum_Major_Glx)))
		{
			KWARN("Current GLX version is %d.%d, but only %d.%d or greater is supported.", major_glx, minor_glx, Minimum_Major_Glx, Minimum_Minor_Glx);
		}
		else
		{
			KINFO("Current GLX version: %d.%d.", major_glx, minor_glx);
		}
	}
	else
	{
		KERROR("Could not retrieve the GLX version.");
	}

	int screen_index = DefaultScreen(display);

	const int Framebuffer_Attribs[] =
	{
		GLX_DOUBLEBUFFER, True,
		GLX_X_RENDERABLE, True,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_SAMPLE_BUFFERS, 1,
		GLX_SAMPLES, 4,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_ALPHA_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		None		
	};

	int fbconfig_options_count = 0;
	GLXFBConfig *fbconfig_options = glXChooseFBConfig(display, screen_index, Framebuffer_Attribs, &fbconfig_options_count);
	if ((fbconfig_options == NULL) || (fbconfig_options_count <= 0))
	{
		KFATAL("Could not retrieve any valid GLX framebuffer configuration.");
		return 1;
	}
	else
	{
		KDEBUG("Retrieved %d GLX framebuffer configuration candidates.", fbconfig_options_count);
	}

	int fbconfig_best_index = -1;
	int fbconfig_best_samples = 0;
	int fbconfig_best_sample_buffers = 0;
	for (int i = 0; i < fbconfig_options_count; i++)
	{
		GLXFBConfig fbconfig = fbconfig_options[i];

		XVisualInfo *visual_info = glXGetVisualFromFBConfig(display, fbconfig);
		if (visual_info == NULL)
		{
			continue;
		}
		XFree(visual_info);

		int samples = 0;
		if (glXGetFBConfigAttrib(display, fbconfig, GLX_SAMPLES, &samples) != 0)
		{
			continue;
		}
		int sample_buffers = 0;
		if (glXGetFBConfigAttrib(display, fbconfig, GLX_SAMPLE_BUFFERS, &sample_buffers) != 0)
		{
			continue;
		}

		if ((samples > fbconfig_best_samples) && (sample_buffers >= 1))
		{
			fbconfig_best_index = i;
			fbconfig_best_samples = samples;
			fbconfig_best_sample_buffers = sample_buffers;
		}
	}

	if (fbconfig_best_index <= -1)
	{
		KWARN("Could not find a compatible GLX framebuffer configuration in %d available options.", fbconfig_options_count);
		fbconfig_best_index = 0;
	}
	GLXFBConfig fbconfig = fbconfig_options[fbconfig_best_index];
	XFree(fbconfig_options);
	fbconfig_options = NULL;
	KDEBUG("GLX Framebuffer configuration: Samples = %d, Sample buffers = %d.", fbconfig_best_samples, fbconfig_best_sample_buffers);

	Window root_window = RootWindow(display, screen_index);
	XVisualInfo *visual_info = glXGetVisualFromFBConfig(display, fbconfig);

	int window_attribs_mask = CWBorderPixel | CWColormap | CWEventMask;
	XSetWindowAttributes window_attribs = {0};
	window_attribs.border_pixel = 0;
	window_attribs.event_mask = StructureNotifyMask;

	window_attribs.colormap = XCreateColormap(display, root_window, visual_info->visual, AllocNone);
	if (window_attribs.colormap == 0)
	{
		KFATAL("Could not create the X11 window colormap.");
		return 1;
	}

	Screen *screen = ScreenOfDisplay(display, screen_index);
	int screen_width = WidthOfScreen(screen);
	int screen_height = HeightOfScreen(screen);
	if (XineramaIsActive(display) == True)
	{
		int screen_infos_count = 0;
		XineramaScreenInfo *screen_infos = XineramaQueryScreens(display, &screen_infos_count);
		if ((screen_infos != NULL) && (screen_infos_count >= 1))
		{
			screen_width = screen_infos[0].width;
			screen_height = screen_infos[0].height;
			XFree(screen_infos);
		}
		else
		{
			KWARN("Could not retrieve the correct Xinerama screen dimensions.");
		}
	}
	KDEBUG("Screen dimensions: Width = %d, Height = %d.", screen_width, screen_height);

	int window_width = screen_width * (3.0f / 4.0f);
	int window_height = screen_height * (3.0f / 4.0f);
	int window_x = (screen_width - window_width) / 2;
	int window_y = (screen_height - window_height) / 2;
	int window_border_width = 0;
	int window_class = InputOutput;
	const char *window_title = "Knave";

	Window window = XCreateWindow(display, root_window, window_x, window_y, window_width, window_height, window_border_width, visual_info->depth, window_class, visual_info->visual, window_attribs_mask, &window_attribs);
	if (window == 0)
	{
		KFATAL("Could not create the X11 window.");
		return 1;
	}
	XStoreName(display, window, window_title);

	Atom window_close_message = XInternAtom(display, "WM_DELETE_WINDOW", False);
	if (XSetWMProtocols(display, window, &window_close_message, 1) == False)
	{
		KERROR("Could not set the X11/WM protocol for closing the window (WM_DELETE_WINDOW).");
	}

	GLXContext context = NULL;
	bool context_is_modern = true;

	const int Minimum_Opengl_Major = 3;
	const int Minimum_Opengl_Minor = 3;

	typedef GLXContext(*Glx_Create_Context_Attribs_Arb)(Display *display, GLXFBConfig fbconfig, GLXContext shared_context, Bool is_direct, const int attribs[]);
	Glx_Create_Context_Attribs_Arb glXCreateContextAttribsARB = (Glx_Create_Context_Attribs_Arb)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
	if (glXCreateContextAttribsARB != NULL)
	{
		const int Context_Attribs[] =
		{
			GLX_CONTEXT_MAJOR_VERSION_ARB, Minimum_Opengl_Major,
			GLX_CONTEXT_MINOR_VERSION_ARB, Minimum_Opengl_Minor,
			GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
			None
		};
		
		context = glXCreateContextAttribsARB(display, fbconfig, NULL, True, Context_Attribs);
		if (context != NULL)
		{
			if (glXMakeCurrent(display, window, context))
			{
				KDEBUG("Modern OpenGL context created.");
			}
			else
			{
				KERROR("Could not make the GLX modern context current.");
				glXDestroyContext(display, context);
				context = NULL;
				glXCreateContextAttribsARB = NULL;
				context_is_modern = false;
			}
		}
		else
		{
			KERROR("Could not create the GLX modern context.");
			glXCreateContextAttribsARB = NULL;
			context_is_modern = false;
		}
	}
	else
	{
		KERROR("Could not load the GLX modern context constructor function.");
		context_is_modern = false;
	}

	if (!context_is_modern)
	{
		context = glXCreateContext(display, visual_info, NULL, True);
		if (!context)
		{
			KFATAL("Could not create the GLX legacy context.");
			return 1;
		}

		if (!glXMakeCurrent(display, window, context))
		{
			KFATAL("Could not make the GLX legacy context current.");
			return 1;
		}

		KWARN("Fallbacking to a legacy GLX context.");
	}

	int opengl_major = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &opengl_major);
	int opengl_minor = 0;
	glGetIntegerv(GL_MINOR_VERSION, &opengl_minor);
	if ((opengl_major < Minimum_Opengl_Major) || ((opengl_minor < Minimum_Opengl_Minor) && (opengl_major == Minimum_Opengl_Major)))
	{
		KWARN("Current OpenGL version is %d.%d, but only %d.%d or greater is supported.", opengl_major, opengl_minor, Minimum_Opengl_Major, Minimum_Opengl_Minor);
	}
	else
	{
		KINFO("OpenGL version: %s.", glGetString(GL_VERSION));
	}
	KINFO("OpenGL renderer: %s", glGetString(GL_RENDERER));
	KINFO("OpenGL vendor: %s", glGetString(GL_VENDOR));

	typedef void(*Glx_Swap_Interval_Ext)(Display *display, GLXDrawable drawable, int interval);
	Glx_Swap_Interval_Ext glXSwapIntervalEXT = (Glx_Swap_Interval_Ext)glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalEXT");
	if (glXSwapIntervalEXT)
	{
		int disable_vsync = 0;
		glXSwapIntervalEXT(display, window, disable_vsync);
		KDEBUG("OpenGL VSync disabled.");
	}
	else
	{
		KERROR("Could not load the GLX VSync disable function.");
	}

	XMapWindow(display, window);
	XMoveWindow(display, window, window_x, window_y);
	
	KTRACE("Main game loop begin.");

	Ku64 first_nanosecond = klin_get_nanosecond();
	Ku64 previous_nanosecond = first_nanosecond;
	Ku64 last_logged_nanosecond = first_nanosecond;
	
	bool keep_running = true;
	while (keep_running)
	{
		for (int i = 0; (i < 16) && (XPending(display) >= 1); i++)
		{
			XEvent event;
			XNextEvent(display, &event);

			switch (event.type)
			{
				case ClientMessage:
				{
					Atom message = (Atom)event.xclient.data.l[0];
					if (message == window_close_message)
					{
						keep_running = false;
						KTRACE("User closed the window.");
					}
				}
				break;

				case ConfigureNotify:
				{
					window_x = event.xconfigure.x;
					window_y = event.xconfigure.y;
					window_width = event.xconfigure.width;
					window_height = event.xconfigure.height;

					glViewport(0, 0, window_width, window_height);
				}
				break;
			}
		}

		Ku64 current_nanosecond = klin_get_nanosecond();
		Ku64 elapsed_nanoseconds = current_nanosecond - previous_nanosecond;
		previous_nanosecond = current_nanosecond;
		Kf64 elapsed_seconds = (Kf64)elapsed_nanoseconds / (Kf64)K_NANOSECONDS_PER_SECOND;

#if 1
		if ((current_nanosecond - last_logged_nanosecond) >= K_NANOSECONDS_PER_SECOND)
		{
			last_logged_nanosecond = current_nanosecond;
			Kf64 current_second = ((Kf64)(current_nanosecond - first_nanosecond) / (Kf64)K_NANOSECONDS_PER_SECOND);
			Kf64 miliseconds = elapsed_seconds * (Kf64)K_MILISECONDS_PER_SECOND;
			Kf64 frames_per_second = 1.0 / elapsed_seconds;
			KTRACE("Second #%.0f: %.2f ms, %.2f fps.", current_second, miliseconds, frames_per_second);
		}
#endif

		KGL_CALL(glClearColor(0.17f, 0.17f, 0.17f, 1.0f));
		KGL_CALL(glClear(GL_COLOR_BUFFER_BIT));
		
		glXSwapBuffers(display, window);
	}

	KTRACE("Main game loop end.");
	
	return 0;
}

bool kos_read_entire_file(Kos_File *file)
{
	int descriptor = open(file->path, O_RDONLY);
	if (descriptor <= -1)
	{
		KERROR("Could not open file %s.", file->path);
		return false;
	}

	struct stat stats;
	if ((fstat(descriptor, &stats) <= -1) || (stats.st_size == 0))
	{
		KERROR("Could not count size of file %s.", file->path);
		close(descriptor);
		return false;
	}

	char *content = mmap(NULL, stats.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, descriptor, 0);
	close(descriptor);
	if ((content == NULL) || (content == MAP_FAILED))
	{
		KERROR("Could not read file %s.", file->path);
		return false;
	}

	file->content = content;
	file->size = stats.st_size;
	return true;
}

void kos_free_file(Kos_File *file)
{
	if (file->content)
	{
		if (munmap(file->content, file->size) <= -1)
		{
			KERROR("Failed to deallocate file %s.", file->path);
		}
		file->content = NULL;
		file->size = 0;
	}
	else
	{
		KWARN("Tryed to free unallocated file %s.", file->path);
	}
}
