#pragma once
#include "CodaIntrinsics.inl"

namespace BGSEditorExtender
{
	namespace BGSEEScript
	{
		// boost declarations are here to prevent adding unnecessary global dependencies
		typedef boost::shared_ptr<ICodaScriptArrayDataType>				CodaScriptSharedHandleArrayT;
		typedef boost::shared_ptr<ICodaScriptDataStoreOwner>			CodaScriptScopedHandleDataStoreOwnerT;
		typedef boost::shared_ptr<ICodaScriptDataStore>					CodaScriptScopedHandleDataStoreT;
		typedef boost::shared_array<ICodaScriptDataStore>				CodaScriptScopedHandleDataStoreArrayT;

		class CodaScriptBackingStore : public ICodaScriptObject, public ICodaScriptDataStore
		{
			friend class CodaScriptVM;

			static int												GIC;
		protected:
			union
			{
				CodaScriptNumericDataTypeT							NumericData;
				CodaScriptReferenceDataTypeT						RefData;
				CodaScriptStringDataTypeT							StringData;
			};
			CodaScriptSharedHandleArrayT							ArrayData;				// not a trivial data type, so can't be a part of the union

			void													Reset(void);
			void													Copy(const CodaScriptBackingStore& Source);
		public:
			CodaScriptBackingStore();
			explicit CodaScriptBackingStore(CodaScriptBackingStore* Data);
			explicit CodaScriptBackingStore(CodaScriptNumericDataTypeT Num);
			explicit CodaScriptBackingStore(CodaScriptStringParameterTypeT Str);
			explicit CodaScriptBackingStore(CodaScriptReferenceDataTypeT Form);
			explicit CodaScriptBackingStore(CodaScriptSharedHandleArrayT Array);
			virtual ~CodaScriptBackingStore();

			explicit CodaScriptBackingStore(const CodaScriptBackingStore& rhs);
			CodaScriptBackingStore& operator=(const CodaScriptBackingStore& rhs);

			CodaScriptBackingStore& operator+=(const CodaScriptBackingStore &rhs);
			CodaScriptBackingStore& operator-=(const CodaScriptBackingStore &rhs);
			CodaScriptBackingStore& operator*=(const CodaScriptBackingStore &rhs);

			virtual CodaScriptReferenceDataTypeT					GetFormID() const;
			virtual CodaScriptNumericDataTypeT						GetNumber() const;
			virtual CodaScriptStringParameterTypeT					GetString() const;
			CodaScriptSharedHandleArrayT							GetArray() const;

			virtual void											SetFormID(CodaScriptReferenceDataTypeT Data);
			virtual void											SetNumber(CodaScriptNumericDataTypeT Data);
			virtual void											SetString(CodaScriptStringParameterTypeT Data);
			virtual void											SetArray(ICodaScriptDataStore* Data);					// fuck-ugly workaround for CRT state inconsistencies during runtime
			void													SetArray(CodaScriptSharedHandleArrayT Data);

			virtual ICodaScriptDataStore&							operator=(const ICodaScriptDataStore& rhs);
			virtual ICodaScriptDataStore&							operator=(CodaScriptNumericDataTypeT Num);
			virtual ICodaScriptDataStore&							operator=(CodaScriptStringParameterTypeT Str);
			virtual ICodaScriptDataStore&							operator=(CodaScriptReferenceDataTypeT Form);
		};

		class ICodaScriptArrayDataType
		{
		public:
			virtual ~ICodaScriptArrayDataType() = 0;

			virtual bool											Insert(CodaScriptBackingStore* Data, int Index = -1) = 0;
			virtual bool											Insert(CodaScriptNumericDataTypeT Data, int Index = -1) = 0;
			virtual bool											Insert(CodaScriptStringParameterTypeT Data, int Index = -1) = 0;
			virtual bool											Insert(CodaScriptReferenceDataTypeT Data, int Index = -1) = 0;
			virtual bool											Insert(CodaScriptSharedHandleArrayT Data, int Index = -1) = 0;

			virtual bool											Erase(UInt32 Index) = 0;
			virtual void											Clear(void) = 0;

			virtual bool											At(UInt32 Index, CodaScriptBackingStore& OutBuffer) const = 0;
			virtual UInt32											Size(void) const = 0;
		};

		class CodaScriptVariable : public ICodaScriptObject
		{
			friend class CodaScriptVM;

			static int												GIC;

			CodaScriptVariable(const CodaScriptVariable& rhs);
			CodaScriptVariable& operator=(const CodaScriptVariable& rhs);
		protected:
			CodaScriptSourceCodeT									Name;
			CodaScriptScopedHandleDataStoreOwnerT					BoundOwner;
		public:
			CodaScriptVariable(CodaScriptSourceCodeT& Name, ICodaScriptDataStoreOwner* Storage);		// claims ownership of the owner
			virtual ~CodaScriptVariable();

			const char*												GetName() const;
			void													SetName(CodaScriptSourceCodeT& Name);
			ICodaScriptDataStoreOwner*								GetStoreOwner() const;
		};
	}
}