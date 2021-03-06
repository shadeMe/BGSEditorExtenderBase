#pragma once

namespace bgsee
{
	namespace script
	{
		class ICodaScriptCommandHandlerHelper;
		class ICodaScriptSyntaxTreeEvaluator;
		class ICodaScriptExpressionByteCode;

		// these typedefs could, of course, be wrapped but what would be the point? I say "meh"
		typedef double													CodaScriptNumericDataTypeT;
		typedef char													CodaScriptCharDataTypeT;
		typedef CodaScriptCharDataTypeT*								CodaScriptStringDataTypeT;
		typedef const CodaScriptCharDataTypeT*							CodaScriptStringParameterTypeT;
		typedef UInt32													CodaScriptReferenceDataTypeT;

		class ICodaScriptDataStore
		{
		public:
			enum DataType
			{
				kDataType_Invalid	= '?',								// default implementation details:
				kDataType_Numeric	= 'n',								//		double precision float
				kDataType_Reference	= 'r',								//		formIDs
				kDataType_String	= 's',								//		c-string
				kDataType_Array		= 'a',								//		ref counted vectors
			};
		protected:
			DataType		Type;
		public:
			ICodaScriptDataStore() : Type(kDataType_Invalid) {}
			virtual ~ICodaScriptDataStore() = 0 {}

			DataType	GetType() const { return Type; }
			bool		IsValid() const { return Type != kDataType_Invalid; }

			bool		IsNumber(bool AllowCasting = true) const
			{
				// only numbers have implicit casts
				return Type == kDataType_Numeric || (AllowCasting && HasImplicitCast(kDataType_Numeric));
			}
			bool		IsReference() const { return (Type == kDataType_Reference); }
			bool		IsString() const { return (Type == kDataType_String); }
			bool		IsArray() const { return (Type == kDataType_Array); }

			virtual bool								HasImplicitCast(DataType NewType) const = 0;
														// the GetXXX accessory functions should perform the necessary casting internally
			virtual CodaScriptReferenceDataTypeT		GetFormID() const = 0;
			virtual CodaScriptNumericDataTypeT			GetNumber() const = 0;
			virtual CodaScriptStringParameterTypeT		GetString() const = 0;

			virtual void								SetFormID(CodaScriptReferenceDataTypeT Data) = 0;
			virtual void								SetNumber(CodaScriptNumericDataTypeT Data) = 0;
			virtual void								SetString(CodaScriptStringParameterTypeT Data) = 0;
			virtual void								SetArray(ICodaScriptDataStore* Data) = 0;

			virtual ICodaScriptDataStore&				operator=(const ICodaScriptDataStore& rhs) = 0;
			virtual ICodaScriptDataStore&				operator=(CodaScriptNumericDataTypeT Num) = 0;
			virtual ICodaScriptDataStore&				operator=(CodaScriptStringParameterTypeT Str) = 0;
			virtual ICodaScriptDataStore&				operator=(CodaScriptReferenceDataTypeT Form) = 0;

			virtual bool								operator==(const ICodaScriptDataStore& rhs) const = 0;
			virtual bool								operator==(const CodaScriptNumericDataTypeT& rhs) const = 0;
			virtual bool								operator==(CodaScriptStringParameterTypeT& rhs) const = 0;
			virtual bool								operator==(const CodaScriptReferenceDataTypeT& rhs) const = 0;

			typedef std::unique_ptr<ICodaScriptDataStore>		PtrT;
		};

		class ICodaScriptDataStoreOwner
		{
		public:
			virtual ~ICodaScriptDataStoreOwner() = 0 {};

			virtual ICodaScriptDataStore*									GetDataStore() = 0;
			virtual ICodaScriptDataStoreOwner&								operator=(const ICodaScriptDataStore& rhs) = 0;
			virtual void													SetIdentifier(const char* Identifier) = 0;

			typedef std::unique_ptr<ICodaScriptDataStoreOwner>				PtrT;
		};

		class ICodaScriptCommand
		{
		public:
			virtual ~ICodaScriptCommand() = 0 {};

			struct ParameterInfo
			{
				const char*							Name;
				UInt8								Type;		// kType_Multi or ICodaScriptDataStore::DataType

				enum
				{
					kType_Multi = '!'
				};
			};

			virtual const char*					GetName(void) = 0;
			virtual const char*					GetAlias(void) = 0;

			virtual const char*					GetDescription(void) = 0;
			virtual const char*					GetDocumentation(void) = 0;		// can contain HTML markup
			virtual int							GetParameterData(int* OutParameterCount = nullptr,
																ParameterInfo** OutParameterInfoArray = nullptr,
																UInt8* OutResultType = nullptr) = 0;
																				// returns parameter count
																				// parameter count must be -1 for variadic functions

			virtual bool						Execute(ICodaScriptDataStore* Arguments,
														ICodaScriptDataStore* Result,
														ParameterInfo* ParameterData,
														int ArgumentCount,
														ICodaScriptCommandHandlerHelper* Utilities,
														ICodaScriptSyntaxTreeEvaluator* ExecutionAgent,
														ICodaScriptExpressionByteCode* ByteCode) = 0;
																				// return false to skip result validation

			typedef std::vector<ICodaScriptCommand*>		ListT;
		};

		class ICodaScriptCommandHandlerHelper
		{
		public:
			virtual ~ICodaScriptCommandHandlerHelper() = 0 {}

			virtual ICodaScriptDataStore*		ArrayAllocate(UInt32 InitialSize = 0) = 0;
			virtual bool						ArrayPushback(ICodaScriptDataStore* AllocatedArray, CodaScriptNumericDataTypeT Data) = 0;
			virtual bool						ArrayPushback(ICodaScriptDataStore* AllocatedArray, CodaScriptStringParameterTypeT Data) = 0;
			virtual bool						ArrayPushback(ICodaScriptDataStore* AllocatedArray, CodaScriptReferenceDataTypeT Data) = 0;
			virtual bool						ArrayPushback(ICodaScriptDataStore* AllocatedArray, ICodaScriptDataStore* ArrayData) = 0;
			virtual bool						ArrayAt(ICodaScriptDataStore* AllocatedArray, UInt32 Index, ICodaScriptDataStore** OutBuffer) = 0;
			virtual bool						ArrayErase(ICodaScriptDataStore* AllocatedArray, UInt32 Index) = 0;
			virtual void						ArrayClear(ICodaScriptDataStore* AllocatedArray) = 0;
			virtual UInt32						ArraySize(ICodaScriptDataStore* AllocatedArray) = 0;

			virtual bool						ExtractArguments(ICodaScriptDataStore* Arguments,
																ICodaScriptCommand::ParameterInfo* ParameterData,
																UInt32 ArgumentCount,
																...) = 0;
																// pass CodaScriptStringParameterTypeT pointers for string arguments
																// ICodaScriptDataStore pointers for arrays and multitype args
																// variadic functions must manually extract their arguments
		};
	}
}