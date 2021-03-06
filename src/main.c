/*!
 * FastPoll - FastCGI Poll
 * @copyright 2015 The FastPoll authors
 */

#include <threads.h> 
#include <sys/types.h> 
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h> 

#include "fsp.h"
#include "app.h"
#include "fcgx.h"
#include "qry.h"

/* number of threads */
#define THREAD_COUNT 20

/* socket fd */
static int sock_id;
static mtx_t mtx;

/**
 * thread callback
 * 
 * @param  a  global application context
 * @return    nothing at all
 */
static void serve(void *a)
{
  /* restore void-pointer */
  struct fsp_app *app = a;
  
  /* init request */
  struct fcgx_req req;
  
  if (fcgx_req_init(&req, sock_id, 0) != 0) {
    puts("error: could not initialize fcgi request");
    return;
  }
  
  int rc; /* used for fcgx_req_accept() */
  
  /* start accept loop */
  for (;;) {
    mtx_lock(&mtx);
    rc = fcgx_req_accept(&req);
    mtx_unlock(&mtx);
    
    if (rc < 0) {
      puts("warn: cannot accept requests, thread will exit");
      break;
    }
    
    fsp_app_process(app, &req);        
    fcgx_req_finish(&req);
  }
}

/**
 * installs the fcgi handler and worker-threads
 * 
 * @param app   global application context
 */
static void listen(struct fsp_app *app, const char *sock)
{
  thrd_t id[THREAD_COUNT]; 

  /* init fcgi handler */
  fcgx_init();
  
  /* open unix socket */
  umask(0);  
  sock_id = fcgx_open_socket(sock, 2000);
  
  /* spawn threads */
  for (long i = 1; i < THREAD_COUNT; ++i)
    thrd_create(&id[i], serve, app);
  
  /* start server on main thread */
  serve(app);
  
  /* join threads */
  for (long i = 1; i < THREAD_COUNT; ++i)
    thrd_join(id[i], NULL);
}

static inline void emit_tabs(int32_t n)
{ 
  for (; n > 0; --n) 
    fputs("  ", stdout); 
}

static void dump_qry_list(struct fsp_qry_item *list, int32_t tab)
{
  struct fsp_qry_item *item;
  for (item = list; 
       item != NULL; 
       item = item->next) {
    
    emit_tabs(tab);
  
    fputs("name: ", stdout);
    fputs(item->name, stdout);
    fputs(", value: ", stdout);
    
    if (item->type == FSP_QRY_STR)
      fputs(item->value.str_val, stdout);
    else {
      fputs("{\n", stdout);
      dump_qry_list(item->value.map_val, tab + 1);
      emit_tabs(tab);
      fputs("}", stdout);
    }
    
    fputs("\n", stdout);
  }
}

/**
 * main
 * 
 * @return  exit code
 */
int main(int argc, char **argv)
{
  const char *qs =
    "ht=p3&chd=t:60,40&chs=250x100&chl=Hello|World&"
    "a[]=b&a[]=&a[]=c";
  
  struct fsp_qry qry;
  fsp_qry_init(&qry);
  fsp_qry_parse(&qry, qs);
  dump_qry_list(qry.list, 0);
  fsp_qry_destroy(&qry);
  return 0;
  
  /* global app context */
  struct fsp_app app;
  
  if (argc != 2 || !argv[1]) {
    fputs("missing socket-path", stderr);
    return 1;
  }
  
  if (!fsp_app_init(&app))
    return 1;
  
  mtx_init(&mtx, 0);
  
  /* fcgi handler */
  listen(&app, argv[1]);
  
  /* shutdown */
  mtx_destroy(&mtx);
  fsp_app_destroy(&app);
  return 0;
}
