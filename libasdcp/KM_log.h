/*
Copyright (c) 2004-2009, John Hurst
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
  /*! \file    KM_log.h
    \version $Id: KM_log.h,v 1.13 2011/03/05 19:15:35 jhurst Exp $
    \brief   message logging API
  */


#ifndef _KM_LOG_H_
#define _KM_LOG_H_

#include <KM_platform.h>
#include <KM_mutex.h>
#include <KM_util.h>
#include <stdarg.h>
#include <errno.h>
#include <iosfwd>

#define LOG_MSG_IMPL(t) \
  va_list args; \
  va_start(args, fmt); \
  vLogf((t), fmt, &args); \
  va_end(args)

// Returns RESULT_PTR if the given argument is NULL.
# define KM_TEST_NULL_L(p) \
  if ( (p) == 0  ) { \
    DefaultLogSink().Error("NULL pointer in file %s, line %d\n", __FILE__, __LINE__); \
    return Kumu::RESULT_PTR; \
  }

// Returns RESULT_PTR if the given argument is NULL. It then
// assumes that the argument is a pointer to a string and returns
// RESULT_NULL_STR if the first character is '\0'.
//
# define KM_TEST_NULL_STR_L(p) \
  KM_TEST_NULL_L(p); \
  if ( (p)[0] == '\0' ) { \
    DefaultLogSink().Error("Empty string in file %s, line %d\n", __FILE__, __LINE__); \
    return Kumu::RESULT_NULL_STR; \
  }


namespace Kumu
{
  // no log message will exceed this length
  const ui32_t MaxLogLength = 512;

  //---------------------------------------------------------------------------------
  // message logging

  // Log messages are recorded by objects which implement the interface given
  // in the class ILogSink below. The library maintains a pointer to a default
  // log sink which is used by the library to report messages.
  //

  // types of log messages
  enum LogType_t {
    LOG_DEBUG,    // detailed developer info
    LOG_INFO,     // developer info
    LOG_WARN,     // library non-fatal or near-miss error
    LOG_ERROR,    // library fatal error
    LOG_NOTICE,   // application user info
    LOG_ALERT,    // application non-fatal or near-miss error
    LOG_CRIT,     // application fatal error
  };


  // OR these values together to come up with sink filter flags.
  // The default mask is LOG_ALLOW_ALL (all messages).
  const i32_t LOG_ALLOW_DEBUG      = 0x00000001;
  const i32_t LOG_ALLOW_INFO       = 0x00000002;
  const i32_t LOG_ALLOW_WARN       = 0x00000004;
  const i32_t LOG_ALLOW_ERROR      = 0x00000008;
  const i32_t LOG_ALLOW_NOTICE     = 0x00000010;
  const i32_t LOG_ALLOW_ALERT      = 0x00000020;
  const i32_t LOG_ALLOW_CRIT       = 0x00000040;
  const i32_t LOG_ALLOW_NONE       = 0x00000000;
  const i32_t LOG_ALLOW_ALL        = 0x000fffff;

  // options are used to control display format default is 0.
  const i32_t LOG_OPTION_TYPE      = 0x01000000;
  const i32_t LOG_OPTION_TIMESTAMP = 0x02000000;
  const i32_t LOG_OPTION_PID       = 0x04000000;
  const i32_t LOG_OPTION_NONE      = 0x00000000;
  const i32_t LOG_OPTION_ALL       = 0xfff00000;

  // A log message with environmental metadata
 class LogEntry : public IArchive
  {
  public:
    ui32_t      PID;
    Timestamp   EventTime;
    LogType_t   Type;
    std::string Msg;

    LogEntry() {}
    LogEntry(ui32_t pid, LogType_t t, const char* m) : PID(pid), Type(t), Msg(m) { assert(m); }
    virtual ~LogEntry() {}

    // returns true if the message Type is present in the mask
    bool   TestFilter(i32_t mask_value) const;

    // renders the message into outstr using the given dispaly options
    // returns outstr&
    std::string& CreateStringWithOptions(std::string& outstr, i32_t mask_value) const;

    // IArchive
    bool   HasValue() const { return ! Msg.empty(); }
    ui32_t ArchiveLength() const;
    bool   Archive(MemIOWriter* Writer) const;
    bool   Unarchive(MemIOReader* Reader);
  };

  //
  std::basic_ostream<char, std::char_traits<char> >&
    operator<<(std::basic_ostream<char, std::char_traits<char> >& strm, LogEntry const& Entry);


  typedef ArchivableList<LogEntry> LogEntryList;
  
  //
  class ILogSink
    {
    protected:
      i32_t m_filter;
      i32_t m_options;

    public:
    ILogSink() : m_filter(LOG_ALLOW_ALL), m_options(LOG_OPTION_NONE) {}
      virtual ~ILogSink() {}

      void  SetFilterFlag(i32_t f) { m_filter |= f; }
      void  UnsetFilterFlag(i32_t f) { m_filter &= ~f; }
      bool  TestFilterFlag(i32_t f) const  { return ((m_filter & f) == f); }

      void  SetOptionFlag(i32_t o) { m_options |= o; }
      void  UnsetOptionFlag(i32_t o) { m_options &= ~o; }
      bool  TestOptionFlag(i32_t o) const  { return ((m_options & o) == o); }

      // library messages
      void Error(const char* fmt, ...)    { LOG_MSG_IMPL(LOG_ERROR); }
      void Warn(const char* fmt, ...)     { LOG_MSG_IMPL(LOG_WARN);  }
      void Info(const char* fmt, ...)     { LOG_MSG_IMPL(LOG_INFO);  }
      void Debug(const char* fmt, ...)    { LOG_MSG_IMPL(LOG_DEBUG); }

      // application messages
      void Critical(const char* fmt, ...) { LOG_MSG_IMPL(LOG_CRIT); }
      void Alert(const char* fmt, ...)    { LOG_MSG_IMPL(LOG_ALERT); }
      void Notice(const char* fmt, ...)   { LOG_MSG_IMPL(LOG_NOTICE); }

      // message with type
      void Logf(LogType_t type, const char* fmt, ...) { LOG_MSG_IMPL(type); }

      // actual log sink input
      virtual void vLogf(LogType_t, const char*, va_list*);
      virtual void WriteEntry(const LogEntry&) = 0;
    };


  // Sets the internal default sink to the given receiver. If the given value
  // is zero, sets the default sink to the internally allocated stderr sink.
  void SetDefaultLogSink(ILogSink* = 0);

  // Returns the internal default sink.
  ILogSink& DefaultLogSink();


  // Sets a log sink as the default until the object is destroyed.
  // The original default sink is saved and then restored on delete.
  class LogSinkContext
  {
    KM_NO_COPY_CONSTRUCT(LogSinkContext);
    LogSinkContext();
    ILogSink* m_orig;

  public:
    LogSinkContext(ILogSink& sink) {
      m_orig = &DefaultLogSink();
      SetDefaultLogSink(&sink);
    }

    ~LogSinkContext() {
      SetDefaultLogSink(m_orig);
    }
  };

  //------------------------------------------------------------------------------------------
  //

  // write messages to two subordinate log sinks 
  class TeeLogSink : public ILogSink
  {
    KM_NO_COPY_CONSTRUCT(TeeLogSink);
    TeeLogSink();

    ILogSink& m_a;
    ILogSink& m_b;

  public:
    TeeLogSink(ILogSink& a, ILogSink& b) : m_a(a), m_b(b) {}
    virtual ~TeeLogSink() {}

    void WriteEntry(const LogEntry& Entry) {
      m_a.WriteEntry(Entry);
      m_b.WriteEntry(Entry);
    }
  };

  // collect log messages into the given list, does not test filter
  class EntryListLogSink : public ILogSink
  {
    Mutex m_Lock;
    LogEntryList& m_Target;
    KM_NO_COPY_CONSTRUCT(EntryListLogSink);
    EntryListLogSink();

  public:
    EntryListLogSink(LogEntryList& target) : m_Target(target) {}
    virtual ~EntryListLogSink() {}

    void WriteEntry(const LogEntry& Entry);
  };


  // write messages to a POSIX stdio stream
  class StdioLogSink : public ILogSink
    {
      Mutex m_Lock;
      FILE* m_stream;
      KM_NO_COPY_CONSTRUCT(StdioLogSink);

    public:
    StdioLogSink() : m_stream(stderr) {}
    StdioLogSink(FILE* stream) : m_stream(stream) {}
      virtual ~StdioLogSink() {}

    void WriteEntry(const LogEntry&);
    };

#ifdef KM_WIN32
  // write messages to the Win32 debug stream
  class WinDbgLogSink : public ILogSink
    {
      Mutex m_Lock;
      KM_NO_COPY_CONSTRUCT(WinDbgLogSink);

    public:
      WinDbgLogSink() {}
      virtual ~WinDbgLogSink() {}

      void WriteEntry(const LogEntry&);
    };
#endif

#ifndef KM_WIN32
  // write messages to a POSIX file descriptor
  class StreamLogSink : public ILogSink
    {
      Mutex m_Lock;
      int   m_fd;
      KM_NO_COPY_CONSTRUCT(StreamLogSink);
      StreamLogSink();

    public:
      StreamLogSink(int fd) : m_fd(fd) {}
      virtual ~StreamLogSink() {}

      void WriteEntry(const LogEntry&);
    };

  // write messages to the syslog facility
  class SyslogLogSink : public ILogSink
    {
      Mutex m_Lock;
      KM_NO_COPY_CONSTRUCT(SyslogLogSink);
      SyslogLogSink();
  
    public:
      SyslogLogSink(const std::string& source_name, int facility);
      virtual ~SyslogLogSink();
      void WriteEntry(const LogEntry&);
    };

  // convert a string into the appropriate syslog facility id
  int SyslogNameToFacility(const std::string& facility_name);

#endif


} // namespace Kumu

#endif // _KM_LOG_H_

//
// end KM_log.h
//
