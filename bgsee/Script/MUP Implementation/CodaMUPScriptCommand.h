#pragma once
#include "mpICallback.h"
#include "CodaBaseInterfaces.inl"

namespace bgsee
{
	namespace script
	{
		namespace mup
		{
			// a wrapper for ICodaScriptCommands
			class CodaScriptMUPScriptCommand : public ICallback
			{
			protected:
				ICodaScriptCommand*					Parent;
			public:
				CodaScriptMUPScriptCommand(ICodaScriptCommand* Source, bool UseAlias = false);
				virtual ~CodaScriptMUPScriptCommand();

				virtual void						Eval(ptr_val_type& ret, const ptr_val_type *arg, int argc);
				virtual const char_type*			GetDesc() const;
				virtual IToken*						Clone() const;
			};
		}
	}
}