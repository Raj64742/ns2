//
// diffapp.cc     : Base Class for Diffusion Apps and Filters
// author         : Fabio Silva and Padma Haldar
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: diffapp.cc,v 1.3 2002/03/21 19:30:54 haldar Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
//

#include "diffapp.hh"

#ifdef NS_DIFFUSION
int DiffApp::command(int argc, const char*const* argv) {

  //if (argc == 2) {
  //if (strcmp(argv[1], "subscribe") == 0) {
  //  start();
  //  return TCL_OK;
  //}
  //if (strcmp(argv[1], "unsubscribe") == 0) {
  //  dr->unsubscribe(subHandle);
  //exit(0);
  //  }
  //}
  if (argc == 3) {
    if (strcmp(argv[1], "dr") == 0) {
      DiffAppAgent *agent;
      agent = (DiffAppAgent *)TclObject::lookup(argv[2]);
      dr = agent->dr();
      return TCL_OK;
    }
  }
  return Application::command(argc, argv);
}

#else
void DiffApp::usage(char *s){
  diffPrint(DEBUG_ALWAYS, "Usage: %s [-d debug] [-p port] [-f file] [-h]\n\n", s);
  diffPrint(DEBUG_ALWAYS, "\t-d - Sets debug level (0-10)\n");
  diffPrint(DEBUG_ALWAYS, "\t-p - Uses port 'port' to talk to diffusion\n");
  diffPrint(DEBUG_ALWAYS, "\t-f - Specifies a config file\n");
  diffPrint(DEBUG_ALWAYS, "\t-h - Prints this information\n");
  diffPrint(DEBUG_ALWAYS, "\n");
  exit(0);
}

void DiffApp::ParseCommandLine(int argc, char **argv)
{
  int debug_level;
  int opt;
  u_int16_t diff_port = DEFAULT_DIFFUSION_PORT;

  config_file = NULL;
  opterr = 0;
  application_id = strdup(argv[0]);

#ifdef BBN_LOGGER
  InitMainLogger();
#endif // BBN_LOGGER

  while (1){
    opt = getopt(argc, argv, "f:hd:p:");
    switch (opt){

    case 'p':

      diff_port = (u_int16_t) atoi(optarg);
      if ((diff_port < 1024) || (diff_port >= 65535)){
	diffPrint(DEBUG_ALWAYS, "Error: Diffusion port must be between 1024 and 65535 !\n");
	exit(-1);
      }

      break;

    case 'h':

      usage(argv[0]);

      break;

    case 'd':

      debug_level = atoi(optarg);

      if (debug_level < 1 || debug_level > 10){
	diffPrint(DEBUG_ALWAYS, "Error: Debug level outside range or missing !\n");
	usage(argv[0]);
      }

      global_debug_level = debug_level;

      break;

    case 'f':

      if (!strncasecmp(optarg, "-", 1)){
	diffPrint(DEBUG_ALWAYS, "Error: Parameter missing !\n");
	usage(argv[0]);
      }

      config_file = strdup(optarg);

      break;

    case '?':

      diffPrint(DEBUG_ALWAYS,
		"Error: %c isn't a valid option or its parameter is missing !\n", optopt);
      usage(argv[0]);

      break;

    case ':':

      diffPrint(DEBUG_ALWAYS, "Parameter missing !\n");
      usage(argv[0]);

      break;

    }

    if (opt == -1)
      break;
  }

  diffusion_port = diff_port;
}

#endif // NS_DIFFUSION
