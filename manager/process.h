#ifndef _PROCESS_H
#define _PROCESS_H

#include <string>
#include <sstream>
#include "hash_mapWrapper.h"

#include <sys/time.h>
#include <sys/resource.h>

#include <st.h>

#include "hashPointer.h"

class service;
namespace pluton {
  class shmServiceHandler;
}

#include "processExitReason.h"
#include "threadedObject.h"
#include "reportingChannel.h"

class process : public threadedObject {

 public:
  process(service* setS, int setID, const std::string& setName,
	  int acceptSocket, int shmFD, int reportingSocket,
	  pluton::shmServiceHandler* setShmService);
  ~process();

  bool	initialize();
  void	preRun() {};
  bool	run();
  void	runUntilIdle();
  void	initiateShutdownSequence(const char* reason);
  void	completeShutdownSequence();
  void	destroyOffspring(threadedObject* to=0, const char* reason="");

  const char* 	getType() const { return "process"; }

  static bool	startThread(process*);

  const char*   getBackoffReason() const { return _backoffReason; }

  pid_t		getPID() const { return _pid; }
  int 		getID() const { return _id; }
  service* 	getSERVICE() const { return _S; }

  void	trackCosts(const char* function, const pluton::reportingChannel::performanceDetails&);
  void	notifyChildExit(int status, struct rusage& ru);
  bool	childDied() const { return _childHasExitedFlag; }
  static const char*	exitCodeToEnglish(int);
  static const char*	reasonCodeToEnglish(processExit::reason);

  enum shutdownReason { noReason, lostSTDIO, unresponse, idle, serviceShutdown };
  void	setShutdownReason(processExit::reason newReason, bool issueKillFlag=false);

  processExit::reason	getExitReason() const { return _exitReason; }
  time_t		getStartTime() const { return _startTime.tv_sec; }

  // Track object usage so that leakage can be detected

  static void	list(std::ostringstream&, const char* name=0);
  static int	getCurrentObjectCount() { return currentObjectCount; }
  static int	getMaximumObjectCount() { return maximumObjectCount; }

  ////////////////////////////////////////////////////////////

 private:
  process&	operator=(const process& rhs);		// Assign not ok
  process(const process& rhs);				// Copy not ok

  static int		currentObjectCount;
  static int		maximumObjectCount;

  typedef		hash_map<const process*, process*, hashPointer>	trackMap;
  static trackMap	processTracker;

  int	setuidMaybe(const char* execPath) const;
  bool	forkExecChild(int acceptSocket, int shmFD, int notifySocket);
  int	readTheirStderr(st_utime_t waitTime=0);
  bool	waitForChildExit();
  void	consumeFinalStderr();

  // Resource reporting

  void	reportChildExit();
  void	reportChildCosts();


  service*      		_S;
  int				_id;
  std::string			_name;
  pluton::shmServiceHandler*	_shmService;

  processExit::reason   _exitReason;		// Why the process exited
  processExit::reason	_sdReason;		// Why we had to shut down the process
  const char*		_backoffReason;		// Why we can't start a new process

  int		_acceptSocket;
  int		_shmServiceFD;
  int		_reportingSocket;

  st_netfd_t	_stErrorNetFD;
  int		_fildesSTDERR[2];

  ////////////////////////////////////////////////////////////
  // Stderr data from the child is assembled into a log entry
  // delimited by \n and written to our stderr. To avoid pummeling the
  // downstream logger, don't log duplicate lines - much as syslog
  // does with "last entry duplicated 'n' times".
  ////////////////////////////////////////////////////////////

  std::string	_pendingLogEntry;
  std::string	_lastLogLine;
  int		_repeatLogEntryCount;

  pid_t	_pid;
  int	_lastSignalSent;
  bool	_childHasExitedFlag;
  int	_childExitStatus;

  struct rusage	_resourcesUsed;

  // Track performance of process

  static const int      _mtipSize = 12;

  struct timeval	_startTime;
  struct timeval	_endTime;
  long long		_totalTimeInProcess;
  unsigned long 	_maximumTimeInProcess[_mtipSize]; 
  long 			_requestsCompleted;
  float 		_totalRequestLength;		// These get BIG and
  float			_totalResponseLength;		// a float approximation is fine.
  long			_maximumRequestLength;
  long			_maximumResponseLength;
  int			_readErrors;
  int			_writeErrors;
  int			_faultsReported;
};

#endif
