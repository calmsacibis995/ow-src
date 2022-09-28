#include <doc/common.h>

#ident "@(#)objstate.h	1.4 06/11/93 Copyright 1992 Sun Microsystems, Inc."

#ifndef	__OPT_OBJECT_STATE_H__
#define	__OPT_OBJECT_STATE_H__

// class OBJECT_STATE
//
// This is a utility object intended for use by class implementors.
// It provides manual and automatic methods of determining whether
// a given run-time object is in a valid state.
//
// Specifically, this object can be used to automatically detect
// the following errors and cause the program to abort:
//
//	o Operations on non-objects (bad pointers, etc.)
//	o Operations on uninitialized objects
//	o Operations on objects that have been deleted
//	o Deleting non-objects
//	o Deleting objects multiple times
//
// OBJECT_STATE works by providing a fixed set of object states and
// permissible state transitions that allow the above error conditions
// to be detected.  These states and state transitions are described
// below in the descriptions of the individual class methods.
//
class	OBJECT_STATE {

    private:

#define	VALID_MASK	(0xABCD << 16)

	// Possible object states.
	// The unusual bit patterns of these states are designed
	// to reduce the chance that some random chunk of memory
	// could masquerade as a valid object.
	// 
	enum OBJSTATE	{
		OBJ_NOTREADY		= (VALID_MASK | 0x1234),
		OBJ_GETTINGREADY	= (VALID_MASK | 0x4567),
		OBJ_READY		= (VALID_MASK | 0x789a),
		OBJ_UNUSABLE		= (VALID_MASK | 0xabcd),
		OBJ_DELETED		=               0xdef0
	};

	// Current state of this object.
	//
	OBJSTATE		objstate;

	// Is this a valid object?
	// Or just some random chunk of memory?
	//
	BOOL	IsValid() const
		{
			return((BOOL)((objstate & VALID_MASK) == VALID_MASK));
		}

    public:

	// Constructor - sets initial object state to "not ready".
	//
	OBJECT_STATE()
		{
			objstate = OBJ_NOTREADY;
		}

	// Destructor - verifies that this is a valid object,
	// then sets object state to "deleted".
	// Note that once the object is marked "deleted", it is
	// no longer valid and any attempt to access it will result
	// in an assertion failure.
	//
	~OBJECT_STATE()
		{
			assert(IsValid());
			objstate = OBJ_DELETED;
		}

	// Is this object ready for action?
	// Verifies that this is a valid object,
	// and that it has been initialized (or is in the process).
	//
	BOOL	IsReady() const
		{
			assert(IsValid());
			return( (BOOL) (objstate == OBJ_READY  ||
					objstate == OBJ_GETTINGREADY));
		}

	// Is this object not ready?
	// Verifies that this is a valid object,
	// and that it has not been previously initialized.
	//
	BOOL	IsNotReady() const
		{
			assert(IsValid());
			return((BOOL)(objstate == OBJ_NOTREADY));
		}

	// Is this object in the process of getting ready?
	// Verifies that this is a valid object.
	//
	BOOL	IsGettingReady() const
		{
			assert(IsValid());
			return((BOOL)(objstate == OBJ_GETTINGREADY));
		}

	// Mark this object as in the process of being initialized.
	// This operation is valid ONLY if object is currently "not ready".
	//
	void	MarkGettingReady()
		{
			assert(IsNotReady());
			objstate = OBJ_GETTINGREADY;
		}

	// Mark this object as initialized and ready for action.
	// This operation is valid ONLY if object is currently "not ready"
	// or "getting ready".
	//
	void	MarkReady()
		{
			assert(IsNotReady() || IsGettingReady());
			objstate = OBJ_READY;
		}

	// Mark this object as unfit for further use.
	// This operation is valid any time as long as the object is valid.
	// Hereafter, the only valid operation for this object is deletion.
	//
	void	MarkUnusable()
		{
			assert(IsValid());
			objstate = OBJ_UNUSABLE;
		}
};

#endif	__OPT_OBJECT_STATE_H__
