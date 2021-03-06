#include "CodaCompiler.h"
#include "CodaUtilities.h"
#include "CodaVM.h"

namespace bgsee
{
	namespace script
	{
		ICodaScriptExpressionByteCode::ICodaScriptExpressionByteCode(ICodaScriptExecutableCode* SourceCode) :
			Source(SourceCode)
		{
			SME_ASSERT(SourceCode);
		}

		ICodaScriptExecutableCode* ICodaScriptExpressionByteCode::GetSource(void) const
		{
			return Source;
		}

		CodaScriptVariable* ICodaScriptExpressionParser::EvaluateData::GetGlobal(const CodaScriptSourceCodeT& Name) const
		{
			for (auto& Itr : GlobalVariables)
			{
				if (!_stricmp(Itr->GetName(), Name.c_str()))
					return Itr;
			}

			return nullptr;
		}

		void CodaScriptProgram::AddVariable(const CodaScriptSourceCodeT& Name, const CodaScriptSourceCodeT& Initalizer, UInt32 Line)
		{
			if (GetVariable(Name) == nullptr)
			{
				VariableInfo Addend(Name, Initalizer, Line);
				Variables.push_back(Addend);
			}
		}

		const CodaScriptProgram::VariableInfo* CodaScriptProgram::GetVariable(const CodaScriptSourceCodeT& Name) const
		{
			return GetVariable(Name.c_str());
		}

		const CodaScriptProgram::VariableInfo* CodaScriptProgram::GetVariable(const char* Name) const
		{
			for (auto& Itr : Variables)
			{
				if (!_stricmp(Itr.Name.c_str(), Name))
					return &Itr;
			}

			return nullptr;
		}

		void CodaScriptProgram::AddParameter(const VariableInfo* BoundVar)
		{
			ParameterInfo Addend(BoundVar);
			Parameters.push_back(Addend);
		}

		bool CodaScriptProgram::IsParameter(const VariableInfo* Var) const
		{
			for (auto& Itr : Parameters)
			{
				if (Var == Itr.BoundVariable)
					return true;
			}

			return false;
		}

		CodaScriptProgram::CodaScriptProgram(ICodaScriptVirtualMachine* VM, const ResourceLocation& Filepath) :
			Filepath(Filepath),
			Name("<unknown>"),
			Variables(),
			Parameters(),
			PollingInterval(0.0),
			AST(),
			Flags(NULL),
			VM(VM),
			Parser(nullptr),
			Metadata()
		{
			SME_ASSERT(VM);

			Parser = VM->GetParser();
			Parser->RegisterProgram(this);
		}

		CodaScriptProgram::~CodaScriptProgram()
		{
			Parser->DeregisterProgram(this);

			SME_ASSERT(VM->IsProgramExecuting(this) == false);
		}

		bool CodaScriptProgram::IsValid() const
		{
			if (Flags & kFlag_Uncompiled)
				return false;
			else if (Flags & kFlag_CompileError)
				return false;
			else if (Flags & kFlag_InvalidBytecode)
				return false;
			else
				return true;
		}

		void CodaScriptProgram::InvalidateBytecode()
		{
			Flags |= kFlag_InvalidBytecode;
		}

		void CodaScriptProgram::Accept(ICodaScriptSyntaxTreeEvaluator* Visitor) noexcept
		{
			SME_ASSERT(AST);
			AST->Accept(Visitor);
		}

		const ResourceLocation& CodaScriptProgram::GetFilepath() const
		{
			return Filepath;
		}

		const CodaScriptSourceCodeT& CodaScriptProgram::GetName() const
		{
			return Name;
		}

		const CodaScriptVariableNameArrayT& CodaScriptProgram::GetVariables(CodaScriptVariableNameArrayT& OutNames) const
		{
			OutNames.clear();

			OutNames.reserve(Variables.size());
			for (auto& Itr : Variables)
				OutNames.push_back(Itr.Name);

			return OutNames;
		}

		UInt32 CodaScriptProgram::GetVariableCount() const
		{
			return Variables.size();
		}

		bool CodaScriptProgram::HasVariable(const CodaScriptSourceCodeT& Name) const
		{
			return GetVariable(Name) != nullptr;
		}

		const CodaScriptVariableNameArrayT& CodaScriptProgram::GetParameters(CodaScriptVariableNameArrayT& OutNames) const
		{
			OutNames.clear();
			OutNames.reserve(Parameters.size());

			for (auto& Itr : Parameters)
				OutNames.push_back(Itr.BoundVariable->Name);

			return OutNames;
		}

		UInt32 CodaScriptProgram::GetParameterCount() const
		{
			return Parameters.size();
		}

		double CodaScriptProgram::GetPollingInteval() const
		{
			return PollingInterval;
		}

		ICodaScriptExpressionParser* CodaScriptProgram::GetBoundParser() const
		{
			return Parser;
		}

		ICodaScriptCompilerMetadata* CodaScriptProgram::GetCompilerMetadata() const
		{
			return Metadata.get();
		}

		CodaScriptSyntaxTreeCompileVisitor::CodaScriptSyntaxTreeCompileVisitor(ICodaScriptVirtualMachine* VM,
																			   ICodaScriptProgram* Program) :
			ICodaScriptSyntaxTreeEvaluator(VM, VM->GetParser(), Program),
			Failed(false)
		{
			SME_ASSERT(Program->GetBoundParser() == GetParser());
		}

#define CODASCRIPT_COMPILERHNDLR_PROLOG										\
			try																\
			{
#define CODASCRIPT_COMPILERHNDLR_EPILOG										\
				return;														\
			}
#define CODASCRIPT_COMPILERERROR_CATCHER									\
			catch (CodaScriptException& E)									\
			{																\
				VM->GetMessageHandler()->Log("Compiler Error [Script: %s]: %s", Program->GetName().c_str(), E.ToString().c_str());		\
			}																\
			catch (std::exception& E)										\
			{																\
				VM->GetMessageHandler()->Log("Compiler Error [Script: %s]: %s", Program->GetName().c_str(), E.what());		\
			}																\
			catch (...)														\
			{																\
				VM->GetMessageHandler()->Log("Unknown Compiler Error [Script: %s]", Program->GetName().c_str());				\
			}																\
			Failed = true;													\
			return;

		void CodaScriptSyntaxTreeCompileVisitor::Visit(CodaScriptExpression* Node)
		{
			GenericCompile(Node);
		}

		void CodaScriptSyntaxTreeCompileVisitor::Visit(CodaScriptBEGINBlock* Node)
		{
			;//
		}

		void CodaScriptSyntaxTreeCompileVisitor::Visit(CodaScriptIFBlock* Node)
		{
			CODASCRIPT_COMPILERHNDLR_PROLOG

			Parser->Compile(this, Node, &Node->ByteCode);

			if (Node->BranchELSE)
				Node->BranchELSE->Accept(this);

			for (auto Itr : Node->BranchELSEIF)
				Itr->Accept(this);

			Node->Traverse(this);

			CODASCRIPT_COMPILERHNDLR_EPILOG

			CODASCRIPT_COMPILERERROR_CATCHER
		}

		void CodaScriptSyntaxTreeCompileVisitor::Visit(CodaScriptELSEIFBlock* Node)
		{
			GenericCompile(Node);
		}

		void CodaScriptSyntaxTreeCompileVisitor::Visit(CodaScriptELSEBlock* Node)
		{
			;//
		}

		void CodaScriptSyntaxTreeCompileVisitor::Visit(CodaScriptWHILEBlock* Node)
		{
			GenericCompile(Node);
		}

		void CodaScriptSyntaxTreeCompileVisitor::Visit(CodaScriptFOREACHBlock* Node)
		{
			CODASCRIPT_COMPILERHNDLR_PROLOG

			if (Node->IteratorName.empty())
				throw CodaScriptException(Node, "Invalid expression - No iterator specified");
			else if (GetProgram()->HasVariable(Node->GetIteratorName()) == false)
				throw CodaScriptException(Node, "Invalid iterator variable '%s'", Node->GetIteratorName().c_str());

			Parser->Compile(this, Node, &Node->ByteCode);
			Node->Traverse(this);

			CODASCRIPT_COMPILERHNDLR_EPILOG

			CODASCRIPT_COMPILERERROR_CATCHER
		}

		void CodaScriptSyntaxTreeCompileVisitor::GenericCompile(ICodaScriptExecutableCode* Node)
		{
			CODASCRIPT_COMPILERHNDLR_PROLOG

			Parser->Compile(this, Node, &Node->ByteCode);
			Node->Traverse(this);

			CODASCRIPT_COMPILERHNDLR_EPILOG

			CODASCRIPT_COMPILERERROR_CATCHER
		}

		bool CodaScriptSyntaxTreeCompileVisitor::HasFailed(void) const
		{
			return Failed;
		}

		CodaScriptCompiler			CodaScriptCompiler::Instance;

		bool CodaScriptCompiler::GetKeywordInStack(CodaScriptKeywordStackT& Stack, CodaScriptKeywordT Keyword) const
		{
			CodaScriptKeywordStackT StackBuffer(Stack);

			while (StackBuffer.size())
			{
				if (StackBuffer.top() == Keyword)
					return true;
				else
					StackBuffer.pop();
			}

			return false;
		}

		bool CodaScriptCompiler::GetKeywordOnStackTop(CodaScriptKeywordStackT& Stack, CodaScriptKeywordT Keyword) const
		{
			if (Stack.size() && Stack.top() == Keyword)
				return true;
			else
				return false;
		}

		bool CodaScriptCompiler::CheckVariableName(const CodaScriptSourceCodeT& Name) const
		{
			for (auto& Itr : Name)
			{
				if (_ismbcalnum(Itr) == 0 && Itr != '_')
					return false;
			}

			return true;
		}

		void CodaScriptCompiler::Preprocess(std::fstream& SourceCode, CodaScriptCompiler::SourceData& OutPreprocessedCode)
		{
			// Call macro: @<script_path>(<args>)
			// use '.' instead of '\' in the path
			static const char kCallMacro_StartSymbol = '@';
			static const char kLineAppendSymbol = '\\';


			class Reader
			{
				std::fstream&		Stream;
				char				Buffer[0x1000];
				int					Line;
			public:
				Reader(std::fstream& In) : Stream(In), Buffer{}, Line(0) {}

				bool IsEOF() { return Stream.eof(); }
				bool ReadLine(CodaScriptSourceCodeT& Out)
				{
					if (IsEOF())
						return false;
					else
					{
						Line++;
						Stream.getline(Buffer, sizeof(Buffer));
						Out = Buffer;
						return true;
					}
				}
				int GetLineNumber() { return Line; }
			};

			Reader SourceReader(SourceCode);
			bool AppendLine = false;
			int SavedLine = -1;
			int CurrentLine = -1;
			CodaScriptSourceCodeT LineAccum;
			CodaScriptTokenizer Tokenizer;

			while (true)
			{
				CodaScriptSourceCodeT LineBuffer, Sanitized;
				bool StopAppend = false;
				if (SourceReader.ReadLine(LineBuffer))
				{
					CurrentLine = SourceReader.GetLineNumber();
					Tokenizer.Sanitize(LineBuffer, Sanitized,
									   CodaScriptTokenizer::kSanitizeOps_StripComments);

					if (Sanitized.empty() == false)
					{
						if (Sanitized.back() == kLineAppendSymbol)
						{
							if (AppendLine == false)
							{
								// begin appending
								AppendLine = true;
								LineAccum = Sanitized.substr(0, Sanitized.length() - 1);
								SavedLine = CurrentLine;
								continue;
							}
							else
							{
								// strip leading whitespace and continue appending
								LineBuffer = Sanitized;
								Tokenizer.Sanitize(LineBuffer, Sanitized,
												   CodaScriptTokenizer::kSanitizeOps_StripLeadingWhitespace);
								LineAccum.append(Sanitized.substr(0, Sanitized.length() - 1));
								continue;
							}
						}
						else if (AppendLine)
						{
							// stop appending
							StopAppend = true;
						}
					}
				}
				else
					StopAppend = true;

				if (StopAppend)
				{
					// stop appending
					AppendLine = false;
					LineBuffer = Sanitized;
					Tokenizer.Sanitize(LineBuffer, Sanitized,
									   CodaScriptTokenizer::kSanitizeOps_StripLeadingWhitespace);
					LineAccum.append(Sanitized);
					Sanitized = LineAccum;
					CurrentLine = SavedLine;
				}

				if (Sanitized.empty() == false)
				{
					CodaScriptSourceCodeT Processed;
					bool ProcessCall = false;
					CodaScriptSourceCodeT CurrentScriptCall;

					int Index = -1;
					for (auto& Itr : Sanitized)
					{
						Index++;
						if (ProcessCall)
						{
							// process until the opening parenthesis
							if (Itr == '(')
							{
								ProcessCall = false;
								if (Sanitized.length() > Index + 1 && Sanitized[Index + 1] != ')')
									CurrentScriptCall += "\", ";
								else
									CurrentScriptCall += "\"";

								Processed += CurrentScriptCall;
							}
							else if (Itr == '.')
								CurrentScriptCall += "\\\\";
							else
								CurrentScriptCall += Itr;

							continue;
						}

						if (Itr == kCallMacro_StartSymbol)
						{
							SME_ASSERT(ProcessCall == false);
							ProcessCall = true;

							CurrentScriptCall = "call(\"";
							continue;
						}
						else
							Processed += Itr;
					}

					// append the unprocessed line if we couldn't find the opening parenthesis
					if (ProcessCall)
						Processed = Sanitized;

					OutPreprocessedCode.Lines.insert(std::make_pair(CurrentLine, Processed));
				}

				if (SourceReader.IsEOF())
					break;
			}
		}

		CodaScriptProgram* CodaScriptCompiler::GenerateProgram(ICodaScriptVirtualMachine* VirtualMachine,
															   CodaScriptProgram* Instance,
															   SourceData& SourceCode)
		{
			CodaScriptProgram* Out = Instance;

			CodaScriptSourceCodeT SourceLine;
			UInt32 LineNo = 1;
			bool Result = true;
			CodaScriptSourceCodeT LineAccum;
			CodaScriptSimpleInstanceCounter<ICodaScriptExecutableCode> CodeInstanceCounter;

			CodaScriptTokenizer Tokenizer;
			CodaScriptKeywordStackT	BlockStack;
			std::unique_ptr<CodaScriptBEGINBlock> Root;
			ICodaScriptExecutableCode::StackT CodeStack;

			BlockStack.push(CodaScriptTokenizer::kTokenType_Invalid);
			CodeStack.push(nullptr);

			for (auto& Itr : SourceCode.Lines)
			{
				LineNo = Itr.first;
				SourceLine = Itr.second;

				if (Tokenizer.Tokenize(SourceLine, false))
				{
					CodaScriptSourceCodeT FirstToken(Tokenizer.Tokens[0]);
					CodaScriptSourceCodeT SecondToken((Tokenizer.GetParsedTokenCount() > 1) ? Tokenizer.Tokens[1] : "");

					if (LineNo == 1)
					{
						if (Tokenizer.GetFirstTokenKeywordType() == CodaScriptTokenizer::kTokenType_ScriptName)
						{
							if (SecondToken != "")
							{
								Out->Name = SecondToken;

								if (Tokenizer.GetParsedTokenCount() >= 3)
								{
									CodaScriptSourceCodeT ThirdToken(Tokenizer.Tokens[2]);
									if (ThirdToken.length() >= 3)
									{
										ThirdToken.erase(0, 1);
										ThirdToken.erase(ThirdToken.end() - 1);

										double DeclaredInteraval = atof(ThirdToken.c_str());
										if (DeclaredInteraval > 0)
											Out->PollingInterval = DeclaredInteraval;
									}
								}
							}
							else
							{
								VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid script name", LineNo);
								Result = false;
							}
						}
						else
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Scripts should start with a script name declaration", LineNo);
							Result = false;
						}

						LineNo++;
						continue;
					}

					CodaScriptKeywordT FirstKeyword = Tokenizer.GetFirstTokenKeywordType();
					switch (FirstKeyword)
					{
					case CodaScriptTokenizer::kTokenType_Variable:
						if (BlockStack.top() != CodaScriptTokenizer::kTokenType_Invalid)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Variable '%s' declared inside a script block", LineNo, SecondToken.c_str());
							Result = false;
						}
						else if (Out->GetVariable(SecondToken))
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Variable '%s' redeclaration", LineNo, SecondToken.c_str());
							Result = false;
						}
						else if (SecondToken == "" || CheckVariableName(SecondToken) == false)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid variable name - Only alphabets, digits and underscores allowed", LineNo);
							Result = false;
						}
						else if (VirtualMachine->GetGlobal(SecondToken.c_str()))
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Scope conflict - Global variable '%s' exists", LineNo, SecondToken.c_str());
							Result = false;
						}
						else
						{
							CodaScriptSourceCodeT LineBuffer;
							CodaScriptSourceCodeT Initer;
							Tokenizer.Sanitize(CodaScriptSourceCodeT(SourceLine), LineBuffer,
											   CodaScriptTokenizer::kSanitizeOps_StripComments |
											   CodaScriptTokenizer::kSanitizeOps_StripLeadingWhitespace |
											   CodaScriptTokenizer::kSanitizeOps_StripTabCharacters);

							if (Tokenizer.Tokenize(LineBuffer, false))
							{
								if (Tokenizer.GetParsedTokenCount() > 2)
									Initer = LineBuffer.substr(Tokenizer.Indices[1]);
							}

							SME::StringHelpers::MakeLower(SecondToken);
							Out->AddVariable(SecondToken, Initer, LineNo);
						}

						break;
					case CodaScriptTokenizer::kTokenType_Begin:
						if (BlockStack.top() != CodaScriptTokenizer::kTokenType_Invalid)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Nested Begin block", LineNo);
							Result = false;
						}
						else if (Root)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Multiple Begin blocks", LineNo);
							Result = false;
						}
						else
						{
							std::vector<CodaScriptSourceCodeT> ParameterIDs;

							for (CodaScriptTokenizer::ParsedTokenListT::iterator Itr = ++(Tokenizer.Tokens.begin()); Itr != Tokenizer.Tokens.end(); Itr++)
								ParameterIDs.push_back(*Itr);

							if (ParameterIDs.size() > kMaxParameters)
							{
								VirtualMachine->GetMessageHandler()->Log("Line %d: Too many parameters passed", LineNo);
								Result = false;
							}
							else
							{
								for (auto& Itr : ParameterIDs)
								{
									const CodaScriptProgram::VariableInfo* ParamVar = Out->GetVariable(Itr);
									if (ParamVar)
										Out->AddParameter(ParamVar);
									else
									{
										VirtualMachine->GetMessageHandler()->Log("Line %d: Couldn't initialize parameter '%s' - Non-existent variable", LineNo, Itr.c_str());
										Result = false;
									}
								}
							}

							BlockStack.push(CodaScriptTokenizer::kTokenType_Begin);
							if (Result == false)
								break;

							Root.reset(new CodaScriptBEGINBlock(SourceLine, LineNo));
							CodeStack.push(Root.get());
						}

						break;
					case CodaScriptTokenizer::kTokenType_End:
						if (BlockStack.top() != CodaScriptTokenizer::kTokenType_Begin)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid block structure. Command 'End' has no matching 'Begin'", LineNo);
							Result = false;
						}
						else
						{
							if (CodeStack.size())
								CodeStack.pop();

							if (BlockStack.size())
								BlockStack.pop();
						}

						break;
					case CodaScriptTokenizer::kTokenType_While:
						if (BlockStack.top() == CodaScriptTokenizer::kTokenType_Invalid)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid block structure. Script commands must be inside a 'Begin' block", LineNo);
							Result = false;
						}
						else if (Tokenizer.GetParsedTokenCount() < 2)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid condition expression", LineNo);
							Result = false;
						}
						else
						{
							BlockStack.push(CodaScriptTokenizer::kTokenType_Loop);
							if (Result == false)
								break;

							ICodaScriptExecutableCode* NewNode = new CodaScriptWHILEBlock(SourceLine, LineNo);
							ICodaScriptExecutableCode* TopNode = CodeStack.top();
							SME_ASSERT(TopNode && TopNode->GetType() != ICodaScriptExecutableCode::kCodeType_Line_EXPRESSION);
							NewNode->Attach(TopNode);
							CodeStack.push(NewNode);
						}

						break;
					case CodaScriptTokenizer::kTokenType_ForEach:
						if (BlockStack.top() == CodaScriptTokenizer::kTokenType_Invalid)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid block structure. Script commands must be inside a 'Begin' block", LineNo);
							Result = false;
						}
						else if (Tokenizer.GetParsedTokenCount() < 4)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid ForEach expression", LineNo);
							Result = false;
						}
						else
						{
							BlockStack.push(CodaScriptTokenizer::kTokenType_Loop);
							if (Result == false)
								break;

							ICodaScriptExecutableCode* NewNode = new CodaScriptFOREACHBlock(SourceLine, LineNo);
							ICodaScriptExecutableCode* TopNode = CodeStack.top();
							SME_ASSERT(TopNode && TopNode->GetType() != ICodaScriptExecutableCode::kCodeType_Line_EXPRESSION);
							NewNode->Attach(TopNode);
							CodeStack.push(NewNode);
						}

						break;
					case CodaScriptTokenizer::kTokenType_Loop:
						if (BlockStack.top() != CodaScriptTokenizer::kTokenType_Loop)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid block structure. Command 'Loop' has no matching 'While' or 'ForEach'", LineNo);
							Result = false;
						}
						else
						{
							if (CodeStack.size())
								CodeStack.pop();

							if (BlockStack.size())
								BlockStack.pop();
						}

						break;
					case CodaScriptTokenizer::kTokenType_If:
						if (BlockStack.top() == CodaScriptTokenizer::kTokenType_Invalid)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid block structure. Script commands must be inside a 'Begin' block", LineNo);
							Result = false;
						}
						else if (Tokenizer.GetParsedTokenCount() < 2)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid condition expression", LineNo);
							Result = false;
						}
						else
						{
							BlockStack.push(CodaScriptTokenizer::kTokenType_If);
							if (Result == false)
								break;

							ICodaScriptExecutableCode* NewNode = new CodaScriptIFBlock(SourceLine, LineNo);
							ICodaScriptExecutableCode* TopNode = CodeStack.top();
							SME_ASSERT(TopNode && TopNode->GetType() != ICodaScriptExecutableCode::kCodeType_Line_EXPRESSION);
							NewNode->Attach(TopNode);
							CodeStack.push(NewNode);
						}

						break;
					case CodaScriptTokenizer::kTokenType_ElseIf:
					case CodaScriptTokenizer::kTokenType_Else:
						if (BlockStack.top() != CodaScriptTokenizer::kTokenType_If &&
							BlockStack.top() != (CodaScriptTokenizer::kTokenType_If | CodaScriptTokenizer::kTokenType_ElseIf))
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid block structure. Command 'Else(If)' has no/incorrectly matching/matched 'If'/'Else(If)'", LineNo);
							Result = false;
						}
						else if (FirstKeyword == CodaScriptTokenizer::kTokenType_ElseIf && Tokenizer.GetParsedTokenCount() < 2)
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid condition expression", LineNo);
							Result = false;
						}
						else
						{
							if (BlockStack.top() == CodaScriptTokenizer::kTokenType_If)
							{
								BlockStack.pop();		// pop IF
								BlockStack.push(CodaScriptTokenizer::kTokenType_If | CodaScriptTokenizer::kTokenType_ElseIf);

								if (Result == false)
									break;
							}
							else
							{
								if (Result == false)
									break;

								CodeStack.pop();		// pop previous ELSE(IF)
							}

							ICodaScriptExecutableCode* NewNode = nullptr;
							ICodaScriptExecutableCode* TopNode = CodeStack.top();
							SME_ASSERT(TopNode && TopNode->GetType() == ICodaScriptExecutableCode::kCodeType_Block_IF);
							CodaScriptIFBlock* IFBlock = dynamic_cast<CodaScriptIFBlock*>(TopNode);

							if (FirstKeyword == CodaScriptTokenizer::kTokenType_ElseIf)
							{
								CodaScriptELSEIFBlock* ElseIfBlock = new CodaScriptELSEIFBlock(SourceLine, LineNo);
								NewNode = ElseIfBlock;
								IFBlock->BranchELSEIF.push_back(ElseIfBlock);
							}
							else if (IFBlock->BranchELSE == nullptr)
							{
								NewNode = IFBlock->BranchELSE = new CodaScriptELSEBlock(SourceLine, LineNo);
							}
							else
							{
								VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid block structure. Multiple 'Else' blocks", LineNo);
								Result = false;
							}

							if (Result)
								CodeStack.push(NewNode);
						}

						break;
					case CodaScriptTokenizer::kTokenType_EndIf:
						if (BlockStack.top() != CodaScriptTokenizer::kTokenType_If &&
							BlockStack.top() != (CodaScriptTokenizer::kTokenType_If | CodaScriptTokenizer::kTokenType_ElseIf))
						{
							VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid block structure. Command 'EndIf' has no/incorrectly matching/matched 'If'/'Else(If)'", LineNo);
							Result = false;
						}
						else
						{
							UInt32 StackTop = BlockStack.top();

							if (CodeStack.size())
							{
								CodeStack.pop();
								if (CodeStack.size() && StackTop != CodaScriptTokenizer::kTokenType_If)		// pop once again to remove the underlying IF block
									CodeStack.pop();
							}

							if (BlockStack.size())
							{
								BlockStack.pop();
							}
						}

						break;
					default:
						{
							switch (FirstKeyword)
							{
							case CodaScriptTokenizer::kTokenType_Return:
								break;
							case CodaScriptTokenizer::kTokenType_Break:
							case CodaScriptTokenizer::kTokenType_Continue:
								if (!GetKeywordInStack(BlockStack, CodaScriptTokenizer::kTokenType_Loop))
								{
									VirtualMachine->GetMessageHandler()->Log("Line %d: Command 'Break'/'Loop' called outside a loop context", LineNo);
									Result = false;
								}

								break;
							}

							if (BlockStack.top() == CodaScriptTokenizer::kTokenType_Invalid)
							{
								VirtualMachine->GetMessageHandler()->Log("Line %d: Invalid block structure. Script commands must be inside a 'Begin' block", LineNo);
								Result = false;
							}

							if (Result == false)
								break;

							ICodaScriptExecutableCode* NewNode = new CodaScriptExpression(SourceLine, LineNo);
							ICodaScriptExecutableCode* TopNode = CodeStack.top();
							SME_ASSERT(TopNode && TopNode->GetType() != ICodaScriptExecutableCode::kCodeType_Line_EXPRESSION);

							NewNode->Attach(TopNode);
						}

						break;
					}
				}

				LineNo++;
			}

			if (Result == false)
			{
				Root.reset(nullptr);
				Out->Flags |= CodaScriptProgram::kFlag_CompileError;

				if (CodeInstanceCounter.GetCount())
				{
					VirtualMachine->GetMessageHandler()->Log("CodaScriptProgram::GenerateAST - By the Power of Grey-Skull! We are leaking executable code!");
					MessageBeep(MB_ICONERROR);
				}
			}
			else
			{
				SME_ASSERT(Root);
				SME_ASSERT(BlockStack.size() == 1 && BlockStack.top() == CodaScriptTokenizer::kTokenType_Invalid);
				SME_ASSERT(CodeStack.size() == 1 && CodeStack.top() == nullptr);

				// generate code for initializers
				ICodaScriptExecutableCode::ArrayT Initers;
				for (auto& Itr : Out->Variables)
				{
					// don't bother if the variable is bound to a parameter
					if (Out->IsParameter(&Itr) == false && Itr.Initalizer.empty() == false)
					{
						CodaScriptExpression* Initer = new CodaScriptExpression(Itr.Initalizer, Itr.Line);
						Initers.push_back(Initer);
					}
				}

				CodaScriptProgram::ScopedASTPointerT Temp(new CodaScriptAbstractSyntaxTree(Root.release(), Initers));
				Out->AST = std::move(Temp);
			}

			return Out;
		}

		CodaScriptProgram* CodaScriptCompiler::GenerateByteCode(ICodaScriptVirtualMachine* VirtualMachine,
																CodaScriptProgram* In)
		{
			if (In->IsValid() == false)
				return In;

			ICodaScriptExpressionParser::CompileData CompilerInput(VirtualMachine->GetGlobals());
			ICodaScriptCompilerMetadata* CompilerMetadata = nullptr;

			try
			{
				CodaScriptSyntaxTreeCompileVisitor Visitor(VirtualMachine, In);
				VirtualMachine->GetParser()->BeginCompilation(In, CompilerInput);
				In->Accept(&Visitor);		// doesn't throw any exceptions, so the next statement will always be executed
				VirtualMachine->GetParser()->EndCompilation(In, &CompilerMetadata);

				if (Visitor.HasFailed())
					In->Flags |= CodaScriptProgram::kFlag_CompileError;
			}
			catch (CodaScriptException& E)
			{
				VirtualMachine->GetMessageHandler()->Log("Compiler Error [Script: %s]: %s", In->GetName().c_str(), E.ToString().c_str());
				In->Flags |= CodaScriptProgram::kFlag_CompileError;
			}
			catch (...)
			{
				VirtualMachine->GetMessageHandler()->Log("Unknown Compiler Error [Script: %s]", In->GetName().c_str());
				In->Flags |= CodaScriptProgram::kFlag_CompileError;
			}

			if (CompilerMetadata)
				In->Metadata.reset(CompilerMetadata);

			return In;
		}

		CodaScriptProgram* CodaScriptCompiler::Compile(ICodaScriptVirtualMachine* VirtualMachine,
													   const ResourceLocation& Filepath)
		{
			std::unique_ptr<CodaScriptProgram> Out(new CodaScriptProgram(VirtualMachine, Filepath));
			std::fstream InputStream(Filepath(), std::iostream::in);

			if (InputStream.fail() == false)
			{
				SourceData Data;

				VirtualMachine->GetMessageHandler()->Indent();
				Preprocess(InputStream, Data);
				GenerateProgram(VirtualMachine, Out.get(), Data);
				GenerateByteCode(VirtualMachine, Out.get());
				VirtualMachine->GetMessageHandler()->Outdent();
			}
			else
			{
				VirtualMachine->GetMessageHandler()->Log("Couldn't read Coda script @ %s", Filepath().c_str());
				Out->Flags |= CodaScriptProgram::kFlag_Uncompiled;
			}

			return Out.release();
		}
	}
}