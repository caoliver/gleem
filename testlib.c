#include <stdarg.h>
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <err.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <ctype.h>
#include "dm.h"
#include "dm_error.h"
#include "greet.h"

#define PWDFILE "etc/passwd"
#define SHDFILE "etc/shadow"
#define GRPFILE "etc/group"


#define DLFUNC(RETURN_TYPE, ARGUMENT_PROTOTYPE)		\
  union {						\
    void *dlsym;					\
    RETURN_TYPE (*function)ARGUMENT_PROTOTYPE;		\
  }

void deleteresources(struct display *d, Display *dpy)
{
  printf("Called DeleteXloginResources(%lx, dpy)\n", (long)d);
}

static void regclose(int cn)
{
  printf("Called RegisterCloseOnFork(dpy, %d)\n", cn);
}

static void clearclose(int cn)
{
  printf("Called RegisterCloseOnFork(dpy, %d)\n", cn);
}

static void secure(struct display *d, Display *dpy)
{
  printf("Called SecureDisplay(%lx, dpy)\n", (long)d);
}

static void unsecure(struct display *d, Display *dpy)
{
  printf("Called UnsecureDisplay(%lx, dpy)\n", (long)d);
}

static void logoom(const char *fmt, ...)
{
  puts("logoom called!  Whazup?");
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

static void logerror(const char *fmt, ...)
{
  puts("LogError called.");
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

static void debugout(const char *fmt, ...)
{
  puts("Debug called.");
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

static void printenv(char **env)
{
  puts("PrintEnv called.");
  while (*env)
    printf("\t%s\n", *env++);
}

struct group *fgetgrent (FILE *__stream);
struct passwd *fgetpwent (FILE *__stream);

static struct passwd *mygetpwnam(const char *name)
{
  FILE *file;
  struct passwd *ent;

  printf("Called getpwnam(%s)\n", name);
  if (!(file = fopen(PWDFILE, "r")))
    return NULL;
  while ((ent = fgetpwent(file)))
    if (!strcmp(ent->pw_name, name))
      break;
  fclose(file);
  return ent;
}

static void myendpwent()
{
  printf("Called endpwent\n");
}

static struct spwd *mygetspnam(const char *name)
{
  FILE *file;
  struct spwd *ent;

  printf("Called getspnam(%s)\n", name);
  if (!(file = fopen(SHDFILE, "r")))
    return NULL;
  while ((ent = fgetspent(file)))
    if (!strcmp(ent->sp_namp, name))
      break;
  fclose(file);
  return ent;
}

static void myendspent()
{
  printf("Called endspent\n");
}

static FILE *grfile;

static struct group *mygetgrent()
{
  printf("Called getgrent\n");
  if (!grfile && !(grfile = fopen(GRPFILE, "r")))
    return NULL;
  return fgetgrent(grfile);
}

static void mysetgrent()
{
  printf("Called setgrent\n");
  if (!grfile)
    rewind(grfile);
}

static void myendgrent()
{
  printf("Called endgrent\n");
  if (!grfile)
    fclose(grfile);
}

#undef crypt
char *crypt(const char *key, const char *salt);

static char *crypt_wrapper(const char *key, const char *salt)
{
  puts("Called crypt");
  return crypt(key, salt);
}

static int pingserver(struct display *d, Display *altdisplay)
{
  printf("Called PingServer(%lx, %lx)\n", (long)d, (long)altdisplay);
  return 1;
}

static void sessionpingfailed(struct display *d)
{
  printf("Called SessionPingFailed(%ld)\n", (long)d);
}

static void sessionexit(struct display *d, int status, int removeAuth)
{
  printf("Called SessionExit(%ld, %d, %d)\n", (long)d, status, removeAuth);
}

int asprintf(char **strp, const char *fmt, ...);

static char *makeEnv(char *name, char *value)
{
  char    *result;

  asprintf(&result, "%s=%s", name, value);

  if (!result)
    {
      logoom ("makeEnv");
      return NULL;
    }
  return result;
}

static char **mysetenv(char **e, char *name, char *value)
{
  char **new, **old;
  char *newe;
  int envsize;
  int l;

  printf("Called setEnv(%lx, %s, %s)\n", (long)e, name, value); 

  l = strlen (name);
  newe = makeEnv (name, value);
  if (!newe)
    {
      logoom ("setEnv");
      return e;
    }
  if (e)
    {
      for (old = e; *old; old++)
	if ((int)strlen (*old) > l &&
	    !strncmp (*old, name, l) && (*old)[l] == '=')
	  break;
      if (*old)
	{
	  free (*old);
	  *old = newe;
	  return e;
	}
      envsize = old - e;
      new = realloc ((char *) e,
		     (unsigned) ((envsize + 2) * sizeof (char *)));
    }
  else
    {
      envsize = 0;
      new = malloc (2 * sizeof (char *));
    }
  if (!new)
    {
      logoom ("setEnv");
      free (newe);
      return e;
    }
  new[envsize] = newe;
  new[envsize+1] = NULL;
  return new;
}

static char **myputenv(const char *string, char **env)
{
  char *v, *b, *n;
  int nl;

  printf("Called putEnv(%s, %lx)\n", string, (long)env); 

  if ((b = strchr(string, '=')) == NULL)
    return NULL;
  v = b + 1;

  nl = b - string;
  if ((n = malloc(nl + 1)) == NULL)
    {
      logoom ("putAllEnv");
      return NULL;
    }

  strncpy(n, string,nl + 1);
  n[nl] = 0;

  env = mysetenv(env,n,v);
  free(n);
  return env;}

static char **parseargs(char **argv, char *string)
{
  char    *word;
  char    *save;
  char    **newargv;
  int     i;

  i = 0;
  while (argv && argv[i])
    ++i;
  if (!argv)
    {
      argv = malloc (sizeof (char *));
      if (!argv)
	{
	  logoom ("parseArgs");
	  return NULL;
	}
    }
  word = string;
  for (;;)
    {
      if (!*string || isblank (*string))
	{
	  if (word != string)
	    {
	      newargv = realloc ((char *) argv,
				 (unsigned) ((i + 2) * sizeof (char *)));
	      save = malloc ((unsigned) (string - word + 1));
	      if (!newargv || !save)
		{
		  logoom ("parseArgs");
		  free (argv);
		  free (newargv);
		  free (save);
		  return NULL;
		}
	      else
		{
		  argv = newargv;
		}
	      argv[i] = strncpy (save, word, string-word);
	      argv[i][string-word] = '\0';
	      i++;
	    }
	  if (!*string)
	    break;
	  word = string + 1;
	}
      ++string;
    }
  argv[i] = NULL;
  return argv;
}

static int mysource (char **environ, char *file)
{
  printf("Called source(%ld, %s)\n", (long)environ, file);
  printenv(environ);
  return 0;
}

static char **defaultenv (void)
{
  char **env, **exp, *value;
  char **exportList = NULL;

  puts("Called defaultEnv");
  env = NULL;
  for (exp = exportList; exp && *exp; ++exp)
    {
      value = getenv (*exp);
      if (value)
	env = mysetenv (env, *exp, value);
    }
  return env;
}

static char ** systemenv (struct display *d, char *user, char *home)
{
  char **env;

  env = defaultenv ();
  env = mysetenv (env, "DISPLAY", d->name);
  if (home)
    env = mysetenv (env, "HOME", home);
  if (user)
    {
      env = mysetenv (env, "USER", user);
      env = mysetenv (env, "LOGNAME", user);
    }
  env = mysetenv (env, "PATH", d->systemPath);
  env = mysetenv (env, "SHELL", d->systemShell);
  if (d->authFile)
    env = mysetenv (env, "XAUTHORITY", d->authFile);
  if (d->windowPath)
    env = mysetenv (env, "WINDOWPATH", d->windowPath);
  return env;
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
  static struct dlfuncs dlfuncs;
  static struct display display;
  static struct greet_info  greet_info;
  static struct verify_info verify_info; 
  
  system("xrdb -load $HOME/.Xresources;xrdb -merge gleem.conf");

  void *libhandle = dlopen("./libXdmGreet.so", RTLD_NOW);

  display.name = getenv("DISPLAY");

  dlfuncs._LogError = logerror;
  dlfuncs._LogOutOfMem = logoom;
  dlfuncs._Debug = debugout;
  dlfuncs._RegisterCloseOnFork = regclose;
  dlfuncs._ClearCloseOnFork = clearclose;
  dlfuncs._SecureDisplay = secure;
  dlfuncs._UnsecureDisplay = unsecure;
  dlfuncs._printEnv = printenv;
  dlfuncs._crypt = crypt_wrapper;
  dlfuncs._DeleteXloginResources = deleteresources;
  dlfuncs._setgrent = mysetgrent;
  dlfuncs._getgrent = mygetgrent;
  dlfuncs._endgrent = myendgrent;
  dlfuncs._getpwnam = mygetpwnam;
  dlfuncs._endpwent = myendpwent;
  dlfuncs._getspnam = mygetspnam;
  dlfuncs._endspent = myendspent;
  dlfuncs._SessionPingFailed = sessionpingfailed;
  dlfuncs._PingServer = pingserver;
  dlfuncs._SessionExit = sessionexit;
  dlfuncs._setEnv = mysetenv;
  dlfuncs._putEnv = myputenv;
  dlfuncs._parseArgs = parseargs;
  dlfuncs._source = mysource;
  dlfuncs._systemEnv = systemenv;
  dlfuncs._defaultEnv = defaultenv;

  if (!libhandle)
    errx(1, dlerror());

  GreetUser.dlsym = dlsym(libhandle, "GreetUser");
  GreetUser.function(&display, &dpy, &verify_info, &greet_info, &dlfuncs);

  dlclose(libhandle);
  return 0;
}
