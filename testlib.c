#include <X11/Xlib.h>
#include <dlfcn.h>
#include <err.h>
#include "dm.h"
#include "dm_error.h"
#include "greet.h"

#define DLFUNC(RETURN_TYPE, ARGUMENT_PROTOTYPE)		\
  union {						\
    void *dlsym;					\
    RETURN_TYPE (*function)ARGUMENT_PROTOTYPE;		\
  }

int main(int argc, char *argv[])
{
  Display *dpy;
  DLFUNC(greet_user_rtn, (struct display *,
			  Display **,
			  struct verify_info *,
			  struct greet_info *,
			  struct dlfuncs *))
    GreetUser;
  struct dlfuncs dlfuncs;
  struct display display;
  struct greet_info  greet_info;
  struct verify_info verify_info; 
  
  void *libhandle = dlopen("./libXdmGreet.so", RTLD_NOW);

  dlfuncs._LogError = (void *)printf;

  if (!libhandle)
    errx(1, dlerror());

  GreetUser.dlsym = dlsym(libhandle, "GreetUser");
  GreetUser.function(&display, &dpy, &verify_info, &greet_info, &dlfuncs);

  dlclose(libhandle);
  return 0;
}
