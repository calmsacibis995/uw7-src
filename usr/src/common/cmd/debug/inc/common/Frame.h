#ifndef Frame_h
#define Frame_h

#ident	"@(#)debugger:inc/common/Frame.h	1.10"

// A ProcObj instance will always have at least one Frame
// instance (the topmost frame, accessing the hardware registers).
// It will produce others upon request, via the Frame::caller()
// member function.  It is the client's responsibility to ensure
// that the process has not moved before attempting to query a
// Frame instance other than ProcObj::topframe.  That is, no Frame
// instance (other than topframe) remains valid after the process
// steps or runs.  The ProcObj class will delete all Frames other
// than topframe whenever it starts in motion for any reason.
//
//
// OPERATIONS
// Frame(ProcObj*)	Constructor - initializes the topmost frame
//
// Frame(Frame*)	Constructor - initializes the caller's frame
// 		and links it into the list; not public
//
// FrameId id()	returns a one-word struct which identifies the
// 		frame; FrameId's may only be compared for equality
//
// Frame *caller()	returns this frame's caller (constructs it if
// 		necessary) or NULL if at bottom of stack
//
// Frame *callee()	returns this frame's callee or NULL if at 
// 		top of stack
//
// int valid()	returns non-zero if frame is still valid (process has
// 		not moved)
//
// int readreg(RegRef which, Stype what, Itype& dest)
// 		fetches register "which" from the frame, and
// 		puts result as a "what" into "dest".
// 		Returns 0 if successful, non-zero if failure.
// 		May fail if combination of "which" and "what"
// 		is nonsensical, such as requesting an IU reg
// 		as a double.
//
// int writereg(RegRef which, Stype what, Itype& src)
// 		writes "src" as a "what" into register "which"
// 		in the frame.  Returns 0 if successful, non-zero
// 		if failure.  May fail if combination of "which"
// 		and "what" is nonsensical, such as writing a
// 		double into an IU register.
//
// Iaddr getreg(RegRef)
// 		shorthand for   { readreg(..., Saddr, itype);
// 				  return itype.iaddr; }
//
// Iint4 argword(int n)
// 		returns nth argument word (0 based)
//		assumes that frame has been completely setup
//		with prolog information, etc.  i.e., this->caller()
//		has been invoked.
//
// Iint4 quick_argword(int n)
// 		returns nth argument word (0 based)
//		assumes procobj is stopped at entry point to func.
//		on processors where that makes a difference
//
// int nargwds(int &assumed)	
//		returns number of words of arguments, 
//			if feasible.
//		If we need to guess at the number, assumed is set to 1
//		assumes that frame has been completely setup
//		with prolog information, etc.  i.e., this->caller()
//		has been invoked.
//
// Iaddr pc_value()	shorthand for frame specific pc contents
//
// int retaddr(Iaddr &pc, Iaddr &stack, Iaddr &frame)
//		returns previous pc, stack pointer and frame pointer,
//		if it can; returns 1 if it can find previous pc,
//		else 0
//
// int incomplete()	returns 1 if it can't determine where things are
//			on the stack at the current location

#include "Link.h"
#include "Reg.h"
#include "Itype.h"

class Frame;
class ProcObj;

struct framedata;		// opaque to clients

struct frameid;			// opaque to clients

class FrameId {
	frameid 	*id;
public:
			FrameId(Frame * = 0);
			~FrameId();
#ifdef __cplusplus
	int		operator==(const FrameId&) const;
	int		operator!=(const FrameId&) const;
	int		isnull() const { return id == 0; }
#else
	int		operator==(FrameId&);
	int		operator!=(FrameId&);
	int		isnull() { return id == 0; }
#endif
	FrameId &	operator=(const FrameId &);
	void		null();
#if DEBUG
	void		print(char * = 0 );
#endif
};


class Frame : public Link {
	ProcObj		*pobj;
	int		level;	  // 0 is top frame
	unsigned long	epoch;
	framedata	*data;	  // all machine specific
				  // Frame data is accessed 
				  // through this ptr
			Frame(Frame*);	// internal
	void		setup();
	int		fixup_stack(Iaddr &, Iaddr &);
	friend class 	FrameId;  // may need to access member "data" in 
				  // constructor.
public:
			Frame(ProcObj*);
			Frame(Iaddr pc, Iaddr sp, ProcObj *);
				// version of constructor allowing
				// explicit setting of pc and stack
				// pointer for top level frame
			~Frame();
	FrameId		*id();
	Frame		*caller();
	Frame		*callee() { return (Frame *)Link::prev(); }
	int		valid();
	int		readreg(RegRef which, Stype what, Itype& dest);
	int		writereg(RegRef which, Stype what, Itype& src);
	int		readreg(RegRef, RegRef, Stype what, Itype& dest);
	int		writereg(RegRef, RegRef, Stype what, Itype& src);
	Iaddr		getreg(RegRef);
	Iint4		argword(int n);
	Iint4		quick_argword(int n);
	int		nargwds(int &assumed);
	Iaddr 		pc_value();
	int		retaddr(Iaddr &, Iaddr &, Iaddr&);
	int		incomplete();
	int		get_level() { return level; }
};

#endif

// end of Frame.h
