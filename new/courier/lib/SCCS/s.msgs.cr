h61451
s 00054/00000/00000
d D 1.1 83/02/23 13:58:10 cooper 1 0
c date and time created 83/02/23 13:58:10 by cooper
e
u
U
t
T
I 1
-- %M% %I% %G%
-- Definition of Courier messages.
-- These are used for the Courier library routines.

CourierMessages : PROGRAM 0 VERSION 0 =
BEGIN
    CallMessageBody : TYPE = RECORD [
	transactionID : UNSPECIFIED,
	programNumber : LONG CARDINAL,
	versionNumber, procedureValue : CARDINAL,
	procedureArguments : RECORD []		-- procedure-dependent
    ];
    ImplementedVersionNumbers : TYPE = RECORD [
	lowest, highest : CARDINAL
    ];
    RejectionType : TYPE = {
	noSuchProgramNumber (0),
	noSuchVersionNumber (1),
	noSuchProcedureValue (2),
	invalidArgument (3),
	unspecifiedError (65535)
    };
    RejectMessageBody : TYPE = RECORD [
	transactionID : UNSPECIFIED,
	rejectionDetails : CHOICE RejectionType OF {
	    noSuchProgramNumber	=> RECORD [],
	    noSuchVersionNumber	=> ImplementedVersionNumbers,
	    noSuchProcedureValue,
	    invalidArgument,
	    unspecifiedError	=> RECORD []
	}
    ];
    ReturnMessageBody : TYPE = RECORD [
	transactionID : UNSPECIFIED,
	procedureResults : RECORD []		-- procedure-dependent
    ];
    AbortMessageBody : TYPE = RECORD [
	transactionID : UNSPECIFIED,
	errorValue : CARDINAL,
	errorArguments : RECORD []		-- error-dependent
    ];
    MessageType : TYPE = {
	callMessage (0),
	rejectMessage (1),
	returnMessage (2),
	abortMessage (3)
    };
    CourierMessage : TYPE = CHOICE MessageType OF {
	callMessage	=> CallMessageBody,
	rejectMessage	=> RejectMessageBody,
	returnMessage	=> ReturnMessageBody,
	abortMessage	=> AbortMessageBody
    };
END.
E 1
