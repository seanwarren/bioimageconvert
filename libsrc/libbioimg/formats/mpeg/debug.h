#ifndef DEBUG_H
#define DEBUG_H

// $Date: 2008-11-17 17:39:15 -0500 (Mon, 17 Nov 2008) $
// $Revision: 706 $

/*
videoIO: granting easy, flexible, and efficient read/write access to video 
                 files in Matlab on Windows and GNU/Linux platforms.
    
Copyright (c) 2006 Gerald Dalley
  
Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

    Portions of this software link to code licensed under the Gnu General 
    Public License (GPL).  As such, they must be licensed by the more 
    restrictive GPL license rather than this MIT license.  If you compile 
    those files, this library and any code of yours that uses it automatically
    becomes subject to the GPL conditions.  Any source files supplied by 
    this library that bear this restriction are clearly marked with internal
    comments.

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
*/

#include <string>
#include <sstream>
#include <iomanip>

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3)
#define HAVE_GCC_43
#endif

#include <errno.h>
#include <cstring>

#if defined(HAVE_GCC_43)
#if defined(__USE_MISC)
//#undef __USE_MISC
#endif
#endif
#include <cstdlib>
#include <cstdio>


namespace VideoIO 
{

  /** This file contains various debugging macros used in the videoIO 
  *  library.  See the user configuration section for #define statements that 
  *  the user may choose to use or not use.  Except for output controlled by
  *  SUPPRESS_3RDPARTY_STDOUT_MESSAGES, all printed messages here use the
  *  stderr stream and are manually fflush-ed.
  */

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
  // Perhaps this should be __FUNCDNAME__ instead.
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

  /*************************************************************************
   *************************************************************************
   **   U S E R   C O N F I G U R A T I O N
   *************************************************************************
   *************************************************************************/


  /*  The following variables may be defined by the user to control the 
      debugging macros here:

      PRINT_ERRORS
        Causes printing of error messages right when exceptions are thrown by 
        Vr*Throw* and Vr*Check*.  This can be useful when trying to debug 
        without a good debugger.

      PRINT_WARNS   
        Causes printing of warning messages suggesting a problem, but not one
        that seems important enough to interrupt the current operation.

      SUPPRESS_3RDPARTY_STDOUT_MESSAGES
        Some of the third-party libraries we use (e.g. ffmpeg) like to print 
        various informational and warning messages to stdout.  When using 
        piped communication these messages will corrupt our communication 
        paths, breaking the system.  When SUPPRESS_3RDPARTY_STDOUT_MESSAGES is 
        defined, we tell these other libraries to suppress their output to 
        stdout.  Only disable this if you are doing low-level debugging with 
        stand-alone applications that do not communicate using our pipecomm 
        functions.

      PRINT_INFOS  
        Causes printing of occasional information messages, but no extra output
        on common calls such as next, step, seek, and getframe.

      PRINT_TRACES  
        Causes printing of a very verbose stack trace.  This is useful for 
        debugging random crashes or multi-process communication deadlocks.  
        Requires that each traced function/method begin with "TRACE;" (without 
        quotes).  

      PRINT_CHECKS 
        Causes all enabled Vr*Check* calls to print out failure and success 
        messages.  This is often useful in conjunction with PRINT_TRACES for 
        doing debugging.  

      PRINT_VERBOSES
        Causes verbose debugging messages to be printed.  Think of this as 
        PRINT_INFOS for functions that get called very frequently like next, 
        step, seek, and getframe.

        
      ECHO_PIPE_COMMUNICATION
        If enabled, any pipe communcation (e.g. what's used with the  
        ffmpegPopen2 plugin) is logged to disk.  This is useful for debugging 
        communication deadlocks, stdout corruption by 3rd-party libraries, and 
        protocol bugs.  Note that this is likely to generate very large log 
        files very quickly if image frames are being transmitted.
      
      TEXT_COMMUNICATIONS
        If enabled, pipe communication is done in text mode.  This is slower 
        than binary mode, but is sometimes easier for debugging, especially 
        when ECHO_PIPE_COMMUNICATION is defined.
  */
 
// Most users will just want to enable one of these four
//#define HARD_CORE_DEBUGGING
//#define STD_DEBUGGING
//#define STD_DEBUGGING_FAST_COMM
#define STD_RUNNING

#ifdef HARD_CORE_DEBUGGING
#  define PRINT_ERRORS
#  define PRINT_WARNS   
#  define SUPPRESS_3RDPARTY_STDOUT_MESSAGES
#  define PRINT_INFOS   
#  define PRINT_TRACES  
#  define PRINT_CHECKS 
#  define PRINT_VERBOSES
#  define ECHO_PIPE_COMMUNICATION
#  define TEXT_COMMUNICATIONS
#elif defined(STD_DEBUGGING)
#  define PRINT_ERRORS
#  define PRINT_WARNS   
#  define SUPPRESS_3RDPARTY_STDOUT_MESSAGES
#  define PRINT_INFOS   
#  define PRINT_TRACES  
#  define PRINT_CHECKS 
#  define PRINT_VERBOSES
#  define ECHO_PIPE_COMMUNICATION
#  define TEXT_COMMUNICATIONS
#elif defined(STD_DEBUGGING_FAST_COMM)
#  define PRINT_ERRORS
#  define PRINT_WARNS   
#  define SUPPRESS_3RDPARTY_STDOUT_MESSAGES
#  define PRINT_INFOS   
#  define PRINT_TRACES  
#  define PRINT_CHECKS 
#  define PRINT_VERBOSES
//#  undef  ECHO_PIPE_COMMUNICATION
//#  undef  TEXT_COMMUNICATIONS
#elif defined(STD_RUNNING)
//#  undef  PRINT_ERRORS
//#  undef  PRINT_WARNS   
#  define SUPPRESS_3RDPARTY_STDOUT_MESSAGES
//#  undef  PRINT_INFOS   
//#  undef  PRINT_TRACES  
//#  undef  PRINT_CHECKS 
//#  undef  PRINT_VERBOSES
//#  undef  ECHO_PIPE_COMMUNICATION
//#  undef  TEXT_COMMUNICATIONS
#endif

  /*************************************************************************
  *************************************************************************
  **   M E S S A G E   P R I N T I N G   M A C R O S 
  *************************************************************************
  *************************************************************************/

  extern int getThreadId();

  // helper for stuffing an insertion-operator expression into an STL string
#define STRINGCAT(deststr, msg) \
  { std::stringstream msgSS; msgSS << msg; deststr = msgSS.str(); }

  // print an insertion-operator expression to stderr with the thread ID and 
  // stack trace indentation.  This macro should not be used by end-users.
#ifndef MATLAB_MEX_FILE
#  define PRINTMESSAGE(msg) \
  { std::string tpmsg; STRINGCAT(tpmsg, \
  "0x" << std::setw(8) << std::hex << getThreadId() << std::dec \
  << Tracer::indentBuff << msg << "\n"); \
  fprintf(stderr, "%s", tpmsg.c_str()); fflush(stderr); }
#else
#include <mex.h>
#  define PRINTMESSAGE(msg) \
  { std::string tpmsg; STRINGCAT(tpmsg, \
  "0x" << std::setw(8) << std::hex << getThreadId() << std::dec \
  << Tracer::indentBuff << msg << "\n"); \
  mexPrintf("%s", tpmsg.c_str()); mexCallMATLAB(0, NULL, 0, NULL, "drawnow"); }
#endif


#ifdef PRINT_ERRORS
#  define PRINTERROR(msg) PRINTMESSAGE("ERROR: " << msg);
#else
#  define PRINTERROR(msg)
#endif

#ifdef PRINT_WARNS
#  define PRINTWARN(msg) PRINTMESSAGE("Warning: " << msg);
#else
#  define PRINTWARN(msg)
#endif

#ifdef PRINT_INFOS
#  define PRINTINFO(msg) PRINTMESSAGE("Info: " << msg);
#else
#  define PRINTINFO(msg)
#endif

#ifdef PRINT_TRACES
#  define TRACE \
  PRINTMESSAGE(">>>> " << __PRETTY_FUNCTION__ << " (line " << __LINE__ << ")");\
  Tracer tracer(__PRETTY_FUNCTION__);
#else
# define TRACE
#endif

#ifdef PRINT_CHECKS
#  define PRINTCHECKENTER(testCond) \
  PRINTMESSAGE("testing " << #testCond << " (line "<<__LINE__<<")..."); 
#  define PRINTCHECKFAILED  PRINTMESSAGE("FAILED!");
#  define PRINTCHECKSUCCESS PRINTMESSAGE("success.");
#else
#  define PRINTCHECKENTER(testCond)
#  define PRINTCHECKFAILED
#  define PRINTCHECKSUCCESS
#endif

#ifdef PRINT_VERBOSES
#  define VERBOSE(msg) PRINTMESSAGE("Verbose: " << msg);
#else
#  define VERBOSE(msg)
#endif

  /*************************************************************************
  *************************************************************************
  **   E X C E P T I O N   C L A S S E S
  *************************************************************************
  *************************************************************************/

  /** If a VrFatalError is thrown, something really bad has happened and 
   *  there may be rouge threads, corrupted memory, or other bad things.
   *  Throwing one of these a chance for callers to do some error 
   *  reporting and exit the running application.  Fatal errors should
   *  be viewed as non-recoverable.  
   *
   *  Since this class uses a std::string internally, additional bad things
   *  can happen if its copy constructor throws an exception.  In particular,
   *  this class has not been tested for robustness in out-of-memory 
   *  situations.
   */
  class VrFatalError : virtual public std::exception
  {
  public:
    VrFatalError() {};
    VrFatalError(std::string const &msg) : message(msg) {};
    virtual ~VrFatalError() throw() {};
    virtual const char *what() const throw() { return message.c_str(); }
    std::string message;
  };

  /** If a VrRecoverableException is thrown, the thrower was able to leave
   *  the system in a valid state and the caller has the option of 
   *  recovering from the problem (as far as the thrower is concerned).   
   *
   *  Since this class uses a std::string internally, additional bad things
   *  can happen if its copy constructor throws an exception.  In particular,
   *  this class has not been tested for robustness in out-of-memory 
   *  situations.
   */
  class VrRecoverableException : virtual public std::exception
  {
  public:
    VrRecoverableException() {};
    VrRecoverableException(std::string const &msg) : message(msg) {};
    virtual ~VrRecoverableException() throw() {};
    virtual const char *what() const throw() { return message.c_str(); }
    std::string message;
  };


#define VrThrow(ET, msg)                                                  \
  {                                                                       \
    ET myException;                                                       \
    STRINGCAT(myException.message,                                        \
              msg <<                                                      \
              "    Function: " << __PRETTY_FUNCTION__ << "\n"             \
              "    File    : " __FILE__ "\n"                              \
              "    Line    : " << std::dec << __LINE__);                  \
    PRINTERROR(myException.message);                                      \
    throw myException;                                                    \
  }

  /** A helpful macro for generating error messages and throwing a 
  *  VrRecoverableException. The msg argument may be any expression that 
  *  can be given to the insertion operator (<<) of an STL stream.
  *
  *  Example:
  *    if (!canOpenFile(filename)) {
  *      VrRecoverableThrow("ERROR: Could not open " << filename << ".");
  *    }
  *
  *  VrRecoverableThrow is good for making user-friendly exception messages 
  *  and ones that include data (such as the filename that was tested in the 
  *  example above).  For tests that are convenient to write, see the 
  *  Vr*Check* macros.
  */
#define VrRecoverableThrow(msg) VrThrow(VrRecoverableException, msg)

#define VrFatalThrow(msg) VrThrow(VrFatalError, msg)

  /*************************************************************************
  *************************************************************************
  **   E R R O R   C H E C K I N G   M A C R O S
  *************************************************************************
  *************************************************************************/

  /* If you want to run some code and make sure that some condition holds, 
  * write statements of the form:
  *   VrRecoverableCheck(...some code that returns true on success...);
  * If the expression provided to these macros is false, a 
  * VrRecoverableException is thrown.  
  *
  * For example,
  *   try {
  *     VrRecoverableCheck(strlen(myFilename) > 0);
  *     FILE *myfile = NULL;
  *     VrRecoverableCheck((myfile = fopen(myFilename, "r")) != NULL);
  *     ...do something with myfile...
  *   } catch (VrRecoverableException const &e) {
  *     if (myfile != NULL) fclose(myfile);
  *     ...any other required explicit cleanup code (often none is needed)...
  *   }
  *
  * The general check macro has the form
  *   VrGenericCheckMsg(testCond, ThrowType, msg, stdMsg)
  * where testCond is the code to test.  If true computation proceeds as 
  * normal.  If its value of testCond is false, an exception of type 
  * ThrowType is thrown.  Embedded in this exception will be some automatic
  * contextual information, the custom error message msg and a secondary
  * message stdMsg.  ThrowType must have a constructor that takes a 
  * std::string as its argument.
  *
  * Nearly all users will want to use one of the convenience macros:
  *   Vr*IoCheck*: 
  *       Uses strerror(errno) for the stdMsg argument.  This is useful
  *       when checking the results of functions from the C standard
  *       library.
  *
  *   Vr*Check*:
  *       Suppresses the stdMsg argument.
  * 
  *   Vr*Msg:
  *       Includes the msg argument (macros that lack "Msg" in their name
  *       suppress the msg argument.
  *
  *   VrRecoverable*:
  *       Uses VrRecoverableException as the ThrowType
  *
  *   VrFatal*:
  *       Uses VrFatalError as the ThrowType.
  */

#define VrGenericCheckMsg(testCond, ThrowType, msg, stdMsg)                  \
     PRINTCHECKENTER(testCond);                                              \
     if (!(testCond)) {                                                      \
       ThrowType myException;                                                \
       STRINGCAT(myException.message,                                        \
                 msg << "\n" <<                                              \
                 "    Failed assertion : " #testCond "\n"                    \
                 << stdMsg <<                                                \
                 "      Function       : " << __PRETTY_FUNCTION__ << "\n"    \
                 "      File           : " __FILE__ "\n"                     \
                 "      Line           : " << __LINE__);                     \
       PRINTCHECKFAILED("FAILED ASSERT!")  ;                                 \
       throw myException;                                                    \
     } else {                                                                \
       PRINTCHECKSUCCESS("success.");                                        \
     }

#define            VrCheckMsg(testCond, ThrowType, msg)  VrGenericCheckMsg(testCond, ThrowType,              msg << "\n", "")
#define               VrCheck(testCond, ThrowType)       VrGenericCheckMsg(testCond, ThrowType,              "",          "")
#define VrRecoverableCheckMsg(testCond, msg)             VrGenericCheckMsg(testCond, VrRecoverableException, msg << "\n", "")
#define    VrRecoverableCheck(testCond)                  VrGenericCheckMsg(testCond, VrRecoverableException, "",          "")
#define       VrFatalCheckMsg(testCond, msg)             VrGenericCheckMsg(testCond, VrFatalError,           msg << "\n", "")
#define          VrFatalCheck(testCond)                  VrGenericCheckMsg(testCond, VrFatalError,           "",          "")

#define            VrIoCheckMsg(testCond, ThrowType, msg)  { errno = 0; VrGenericCheckMsg(testCond, ThrowType,              msg << "\n", strerror(errno) << "\n"); }
#define               VrIoCheck(testCond, ThrowType)       { errno = 0; VrGenericCheckMsg(testCond, ThrowType,              "",          strerror(errno) << "\n"); }
#define VrRecoverableIoCheckMsg(testCond, msg)             { errno = 0; VrGenericCheckMsg(testCond, VrRecoverableException, msg << "\n", strerror(errno) << "\n"); }
#define    VrRecoverableIoCheck(testCond)                  { errno = 0; VrGenericCheckMsg(testCond, VrRecoverableException, "",          strerror(errno) << "\n"); }
#define       VrFatalIoCheckMsg(testCond, msg)             { errno = 0; VrGenericCheckMsg(testCond, VrFatalError,           msg << "\n", strerror(errno) << "\n"); }
#define          VrFatalIoCheck(testCond)                  { errno = 0; VrGenericCheckMsg(testCond, VrFatalError,           "",          strerror(errno) << "\n"); }

  /*************************************************************************
  *************************************************************************
  **   E X E C U T I O N   T R A C I N G
  *************************************************************************
  *************************************************************************/

  /** Manages the stack trace for the current process.  This class assumes that
  *  only one thread is being traced.  Typical end users should put a call to 
  *  the TRACE macro at the beginning of every function and not use the Tracer
  *  class directly. */
  class Tracer 
  {
  public:
    Tracer(std::string func) : func(func) { indentBuff += "  "; };
    ~Tracer() { 
      indentBuff.resize(indentBuff.size() - 2);
      PRINTMESSAGE("<<<<" << func);
    }
    static std::string indentBuff;
  private:
    static int level;
    std::string func;
  };

}; /* namespace VideoIO */

#endif
