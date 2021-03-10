/*
============================================================================
Name        : DXDebug.h
Author      : CoCoMo
Version     : V0.9.0002
Copyright   : Dayhand Network Co, Ltd.
Description : 
============================================================================
*/

#ifndef __DXDEBUG_H__
#define __DXDEBUG_H__

#if (defined _DEBUG && defined EKA2)
#define SUPPORT_DEBUG_PRINT				  //标准输出
#endif
#define SUPPORT_DEBUG_FILE                //文件输出
#if (defined _DEBUG)
#define SUPPORT_DEBUG_ASSERT              //开启ASSERT
#endif

#if    defined (SUPPORT_DEBUG_PRINT)\
	|| defined (SUPPORT_DEBUG_FILE)\
	|| defined (SUPPORT_DEBUG_ASSERT)
#      define SUPPORT_DEBUG_STANDARD
#endif

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)

#ifdef SUPPORT_DEBUG_PRINT
#      include <e32svr.h> //RDebug
#endif

#ifdef SUPPORT_DEBUG_FILE
#      include <f32file.h> //RFile
#endif

#include <EIKENV.H>
#include <CHARCONV.H>

//===================================================================

#ifdef SUPPORT_DEBUG_STANDARD
namespace Dbg
{
#ifdef SUPPORT_DEBUG_FILE
#ifdef EKA2
	_LIT(KDebugFile, "C:\\Data\\DevLogger.txt");
#else
	_LIT(KDebugFile, "C:\\Nokia\\DevLogger.txt");
#endif
#endif

	//===================================================================
	class TDEBUGOverflow8 : public TDes8Overflow
	{
	public:
		void Overflow(TDes8& aDes) { aDes.Zero(); }
	};

	class TDEBUGOverflow16 : public TDes16Overflow
	{
	public:
		void Overflow(TDes16& aDes) { aDes.Zero(); }
	};

	inline void ConvUni2Gbk(const TDesC& original, TDes8& res) 
	{
		TInt state = CCnvCharacterSetConverter::KStateDefault;
		CCnvCharacterSetConverter* iConv = CCnvCharacterSetConverter::NewLC();
		if(iConv->PrepareToConvertToOrFromL(KCharacterSetIdentifierGbk,	CEikonEnv::Static()->FsSession()) != CCnvCharacterSetConverter::EAvailable)
			User::Leave(KErrNotSupported);
		iConv->ConvertFromUnicode(res, original, state);
		CleanupStack::PopAndDestroy();
	}

#ifdef SUPPORT_DEBUG_PRINT
	//===================================================================
	inline void Print8(TRefByValue<const TDesC8> aFmt, ...)
	{
		VA_LIST list;
		VA_START(list, aFmt);

		TDEBUGOverflow8 _overflow;
		HBufC8* _pBuf = NULL;
		for (TInt i = 0; i < 100; i++) //20K
		{
			_pBuf = HBufC8::New(256 * (i + 1));
			if(!_pBuf)
				break;
			
			_pBuf->Des().AppendFormatList(aFmt, list, &_overflow);

			if(_pBuf->Length() > 0)
				break;

			delete _pBuf;
			_pBuf = NULL;
		}

		VA_END(list);

		if(!_pBuf)
		{
			RDebug::Printf("CoCoMo %s(%d) ### Overflow 20K ###", __FILE__, __LINE__);
			return;
		}

		TInt _count = _pBuf->Length() / 200;
		for (TInt i = 0; i < _count; i++)
		{
			TPtrC8 ptr = _pBuf->Mid(i * 200).Left(200);

			if(i == 0)
				RDebug::Printf("%S", &ptr);
			else
				RDebug::Printf("CoCoMo ## %S", &ptr);
		}

		if(_count * 200 < _pBuf->Length())
		{
			TPtrC8 ptr = _pBuf->Mid(_count * 200).Left(200);

			if(_count == 0)
				RDebug::Printf("%S", &ptr);
			else
				RDebug::Printf("CoCoMo ## %S", &ptr);
		}

		delete _pBuf;
		_pBuf = NULL;
	}

	inline void Print16(TRefByValue<const TDesC> aFmt, ...)
	{
		VA_LIST list;
		VA_START(list, aFmt);

		TDEBUGOverflow16 _overflow;
		HBufC* _pBuf = NULL;
		for (TInt i = 0; i < 100; i++) //20K
		{
			_pBuf = HBufC::New(256 * (i + 1));
			if(!_pBuf)
				break;

			_pBuf->Des().AppendFormatList(aFmt, list, &_overflow);

			if(_pBuf->Length() > 0)
				break;

			delete _pBuf;
			_pBuf = NULL;
		}

		VA_END(list);

		if(!_pBuf)
		{
			RDebug::Print(_L("CoCoMo %s(%d) ### Overflow 20K ###"), __WFILE__, __LINE__);
			return;
		}

		TInt _count = _pBuf->Length() / 200;
		for (TInt i = 0; i < _count; i++)
		{
			TPtrC ptr = _pBuf->Mid(i * 200).Left(200);

			if(i == 0)
				RDebug::Print(_L("%S"), &ptr);
			else
				RDebug::Print(_L("CoCoMo ## %S"), &ptr);
		}

		if(_count * 200 < _pBuf->Length())
		{
			TPtrC ptr = _pBuf->Mid(_count * 200).Left(200);

			if(_count == 0)
				RDebug::Print(_L("%S"), &ptr);
			else
				RDebug::Print(_L("CoCoMo ## %S"), &ptr);
		}

		delete _pBuf;
		_pBuf = NULL;
	}

	inline static void PrintGBK(TRefByValue<const TDesC> aFmt, ...)
	{
		VA_LIST list;
		VA_START(list, aFmt);

		TDEBUGOverflow16 _overflow;
		HBufC* _pBuf = NULL;
		for (TInt i = 0; i < 100; i++) //20K
		{
			_pBuf = HBufC::NewLC(256 * (i + 1));

			_pBuf->Des().AppendFormatList(aFmt, list, &_overflow);

			if(_pBuf->Length() > 0)
				break;

			CleanupStack::PopAndDestroy();
			_pBuf = NULL;
		}

		VA_END(list);

		if(!_pBuf)
		{
			RDebug::Print(_L("CoCoMo %s(%d) ### Overflow 20K ###"), __WFILE__, __LINE__);
			return;
		}

		HBufC8* _pBuf8 = HBufC8::NewLC(_pBuf->Length() + 256);
		TPtr8 _szBuf8(_pBuf8->Des());

		ConvUni2Gbk(*_pBuf, _szBuf8);

		CleanupStack::Pop();
		CleanupStack::PopAndDestroy();
		CleanupStack::PushL(_pBuf8);

		TInt _count = _pBuf8->Length() / 200;
		for (TInt i = 0; i < _count; i++)
		{
			TPtrC8 ptr = _pBuf8->Mid(i * 200).Left(200);

			if(i == 0)
				RDebug::Printf("%S", &ptr);
			else
				RDebug::Printf("CoCoMo ## %S", &ptr);
		}

		if(_count * 200 < _pBuf8->Length())
		{
			TPtrC8 ptr = _pBuf8->Mid(_count * 200).Left(200);

			if(_count == 0)
				RDebug::Printf("%S", &ptr);
			else
				RDebug::Printf("CoCoMo ## %S", &ptr);
		}

		CleanupStack::PopAndDestroy(); //_pBuf8
	}
#endif

#ifdef SUPPORT_DEBUG_FILE
	inline void Log8(TRefByValue<const TDesC8> aFmt, ...)
	{
		RFile logFile;

		//Create the file if it doesn't exist and open for writing
		if (logFile.Create(CEikonEnv::Static()->FsSession(), KDebugFile, EFileShareExclusive|EFileStreamText|EFileWrite) != KErrNone) 
		{
			if(logFile.Open(CEikonEnv::Static()->FsSession(), KDebugFile, EFileShareExclusive|EFileStreamText|EFileWrite) != KErrNone)
				return;
		}

		TInt pos(0);
		logFile.Seek(ESeekEnd, pos);

		VA_LIST list;
		VA_START(list, aFmt);

		TDEBUGOverflow8 _overflow;
		HBufC8* _pBuf = NULL;
		for (TInt i = 0; i < 100; i++) //20K
		{
			_pBuf = HBufC8::New(256 * (i + 1));
			if(!_pBuf)
				break;

			_pBuf->Des().AppendFormatList(aFmt, list, &_overflow);

			if(_pBuf->Length() > 0)
				break;

			delete _pBuf;
			_pBuf = NULL;
		}

		VA_END(list);

		TTime time;
		time.HomeTime();

		TBuf<128> dateString;
		_LIT(KDateString,"%D%M%Y%/0%1%/1%2%/2%3%/3 %-B%:0%J%:1%T%:2%S%.%*C4%:3%+B");
		time.FormatL(dateString, KDateString);
		
		if(!_pBuf)
		{
			TBuf8<256> szBuf;
			szBuf.Copy(dateString);
			szBuf.Append(_L8(" :: "));
			szBuf.AppendFormat(_L8("%s(%d) ### Overflow 20K ###\n"), __FILE__, __LINE__);

			logFile.Write(szBuf);

			logFile.Flush();
			logFile.Close();
			return;
		}
		
		HBufC8* _pFileData = HBufC8::New(_pBuf->Length() + 256);
		if(_pFileData)
			{
			_pFileData->Des().Copy(dateString);
			_pFileData->Des().Append(_L8(" :: "));
	//		_pFileData->Des().AppendFormat(_L8("%s(%d) "), __FILE__, __LINE__);
			_pFileData->Des().Append(*_pBuf);
			_pFileData->Des().Append(_L8("\n"));

			logFile.Write(*_pFileData);
			}

		logFile.Flush();
		logFile.Close();

		delete _pBuf;
		_pBuf = NULL;
		delete _pFileData;
		_pFileData = NULL;
	}

	inline void Log16(TRefByValue<const TDesC> aFmt, ...)
	{
		RFile logFile;

		//Create the file if it doesn't exist and open for writing
		if (logFile.Create(CEikonEnv::Static()->FsSession(), KDebugFile, EFileShareExclusive|EFileStreamText|EFileWrite) != KErrNone) 
		{
			if(logFile.Open(CEikonEnv::Static()->FsSession(), KDebugFile, EFileShareExclusive|EFileStreamText|EFileWrite) != KErrNone)
				return;
		}

		TInt pos(0);
		logFile.Seek(ESeekEnd, pos);

		VA_LIST list;
		VA_START(list, aFmt);

		TDEBUGOverflow16 _overflow;
		HBufC* _pBuf = NULL;
		for (TInt i = 0; i < 100; i++) //20K
		{
			_pBuf = HBufC::New(256 * (i + 1));
			if(!_pBuf)
				break;

			_pBuf->Des().AppendFormatList(aFmt, list, &_overflow);

			if(_pBuf->Length() > 0)
				break;

			delete _pBuf;
			_pBuf = NULL;
		}

		VA_END(list);

		TTime time;
		time.HomeTime();

		TBuf<128> dateString;
		_LIT(KDateString,"%D%M%Y%/0%1%/1%2%/2%3%/3 %-B%:0%J%:1%T%:2%S%.%*C4%:3%+B");
		time.FormatL(dateString, KDateString);

		if(!_pBuf)
		{
			TBuf8<256> szBuf;
			szBuf.Copy(dateString);
			szBuf.Append(_L8(" :: "));
			szBuf.AppendFormat(_L8("%s(%d) ### Overflow 20K ###\n"), __FILE__, __LINE__);

			logFile.Write(szBuf);

			logFile.Flush();
			logFile.Close();
			return;
		}

		HBufC8* _pFileData = HBufC8::New(_pBuf->Length() + 256);
		if(_pFileData)
			{
			_pFileData->Des().Copy(dateString);
			_pFileData->Des().Append(_L8(" :: "));
	//		_pFileData->Des().AppendFormat(_L8("%s(%d) "), __FILE__, __LINE__);
			_pFileData->Des().Append(*_pBuf);
			_pFileData->Des().Append(_L8("\n"));

			logFile.Write(*_pFileData);	
			}
		
		logFile.Flush();
		logFile.Close();

		delete _pBuf;
		_pBuf = NULL;
		delete _pFileData;
		_pFileData = NULL;
	}

	inline static void LogGBK(TRefByValue<const TDesC> aFmt, ...)
	{
		RFile logFile;

		//Create the file if it doesn't exist and open for writing
		if (logFile.Create(CEikonEnv::Static()->FsSession(), KDebugFile, EFileShareExclusive|EFileStreamText|EFileWrite) != KErrNone) 
		{
			if(logFile.Open(CEikonEnv::Static()->FsSession(), KDebugFile, EFileShareExclusive|EFileStreamText|EFileWrite) != KErrNone)
				return;
		}

		TInt pos(0);
		logFile.Seek(ESeekEnd, pos);

		VA_LIST list;
		VA_START(list, aFmt);

		TDEBUGOverflow16 _overflow;
		HBufC* _pBuf = NULL;
		for (TInt i = 0; i < 100; i++) //20K
		{
			_pBuf = HBufC::NewLC(256 * (i + 1));

			_pBuf->Des().AppendFormatList(aFmt, list, &_overflow);

			if(_pBuf->Length() > 0)
				break;

			CleanupStack::PopAndDestroy();
			_pBuf = NULL;
		}

		VA_END(list);

		TTime time;
		time.HomeTime();

		TBuf<128> dateString;
		_LIT(KDateString,"%D%M%Y%/0%1%/1%2%/2%3%/3 %-B%:0%J%:1%T%:2%S%.%*C4%:3%+B");
		time.FormatL(dateString, KDateString);

		if(!_pBuf)
		{
			TBuf8<256> szBuf;
			szBuf.Copy(dateString);
			szBuf.Append(_L8(" :: "));
			szBuf.AppendFormat(_L8("%s(%d) ### Overflow 20K ###\n"), __FILE__, __LINE__);

			logFile.Write(szBuf);

			logFile.Flush();
			logFile.Close();
			return;
		}

		HBufC8* _pBuf8 = HBufC8::NewLC(_pBuf->Length() + 256);
		TPtr8 _szBuf8(_pBuf8->Des());

		ConvUni2Gbk(*_pBuf, _szBuf8);

		CleanupStack::Pop();
		CleanupStack::PopAndDestroy();
		CleanupStack::PushL(_pBuf8);

		HBufC8* _pFileData = HBufC8::NewLC(_pBuf8->Length() + 256);

		_pFileData->Des().Copy(dateString);
		_pFileData->Des().Append(_L8(" :: "));
//		_pFileData->Des().AppendFormat(_L8("%s(%d) "), __FILE__, __LINE__);
		_pFileData->Des().Append(*_pBuf8);
		_pFileData->Des().Append(_L8("\n"));

		logFile.Write(*_pFileData);

		logFile.Flush();
		logFile.Close();

		CleanupStack::PopAndDestroy(2);
	}
#endif
}
#endif

//===================================================================
#ifdef SUPPORT_DEBUG_PRINT
#   define DP(MESS);\
	{\
		_LIT8(_KTxtDbg, "CoCoMo %s(%d) " ## MESS);\
		Dbg::Print8(_KTxtDbg, __FILE__, __LINE__);\
	}
#   define DPA(MESS, LIST...);\
	{\
		_LIT8(_KTxtDbg, "CoCoMo %s(%d) " ## MESS);\
		Dbg::Print8(_KTxtDbg, __FILE__, __LINE__, LIST);\
	}
#   define DPU(MESS, LIST...);\
	{\
		_LIT(_KTxtDbg, "CoCoMo %s(%d) " ## MESS);\
		Dbg::Print16(_KTxtDbg, __WFILE__, __LINE__, LIST);\
	}
#   define DPG(MESS, LIST...);\
	{\
		_LIT(_KTxtDbg, "CoCoMo %s(%d) " ## MESS);\
		Dbg::PrintGBK(_KTxtDbg, __WFILE__, __LINE__, LIST);\
	}
#else
#   define DP(MESS);
#   define DPA(MESS, LIST...);
#   define DPU(MESS, LIST...);
#   define DPG(MESS, LIST...);
#endif

#ifdef SUPPORT_DEBUG_FILE
#   define DF(MESS);\
	{\
		_LIT8(_KTxtDbg, "%s(%d) " # MESS);\
		Dbg::Log8(_KTxtDbg, __FILE__, __LINE__);\
	}
#   define DFA(MESS, LIST...);\
	{\
		_LIT8(_KTxtDbg, "%s(%d) " # MESS);\
		Dbg::Log8(_KTxtDbg, __FILE__, __LINE__, LIST);\
	}
#   define DFU(MESS, LIST...);\
	{\
		_LIT(_KTxtDbg, "%s(%d) " # MESS);\
		Dbg::Log16(_KTxtDbg, __WFILE__, __LINE__, LIST);\
	}
#   define DFG(MESS, LIST...);\
	{\
		_LIT(_KTxtDbg, "%s(%d) " # MESS);\
		Dbg::LogGBK(_KTxtDbg, __WFILE__, __LINE__, LIST);\
	}
#else
#   define DF(MESS);
#   define DFA(MESS, LIST...);
#   define DFU(MESS, LIST...);
#   define DFG(MESS, LIST...);
#endif

#if defined (SUPPORT_DEBUG_PRINT) || defined (SUPPORT_DEBUG_FILE)
#   define DPF(MESS);\
	{\
		DP(MESS);\
		DF(MESS);\
	}
#   define DPFA(MESS, LIST...);\
	{\
		DPA(MESS, LIST);\
		DFA(MESS, LIST);\
	}
#   define DPFU(MESS, LIST...);\
	{\
		DPU(MESS, LIST);\
		DFU(MESS, LIST);\
	}
#   define DPFG(MESS, LIST...);\
	{\
		DPG(MESS, LIST);\
		DFG(MESS, LIST);\
	}
#else
#   define DPF(MESS);
#   define DPFA(MESS, LIST...);
#   define DPFU(MESS, LIST...);
#   define DPFG(MESS, LIST...);
#endif

//断言
#ifdef SUPPORT_DEBUG_ASSERT
#   define _ASSERT(x);\
	{\
		if(!(x))\
		{\
			DP("ASSERT FAIL");\
			_LIT(_KTxtFail, "ASSERT FAIL");\
			User::Panic(_KTxtFail, 2);\
		}\
	}
#else
#   define _ASSERT(x);
#endif

//警告
#if defined (SUPPORT_DEBUG_PRINT) || defined (SUPPORT_DEBUG_FILE)
#   define WARNING(MESS);\
	{\
		DP("********************WARNING********************");\
		DF("********************WARNING********************");\
		DP(MESS);\
		DF(MESS);\
		DP("***********************************************");\
		DF("***********************************************");\
	}
#   define WARNINGA(MESS, LIST...);\
	{\
		DP("********************WARNING********************");\
		DF("********************WARNING********************");\
		DPA(MESS, LIST);\
		DFA(MESS, LIST);\
		DP("***********************************************");\
		DF("***********************************************");\
	}
#   define WARNINGU(MESS, LIST...);\
	{\
		DP("********************WARNING********************");\
		DF("********************WARNING********************");\
		DPU(MESS, LIST);\
		DFU(MESS, LIST);\
		DP("***********************************************");\
		DF("***********************************************");\
	}
#   define WARNINGG(MESS, LIST...);\
	{\
		DP("********************WARNING********************");\
		DF("********************WARNING********************");\
		DPG(MESS, LIST);\
		DFG(MESS, LIST);\
		DP("***********************************************");\
		DF("***********************************************");\
	}
#else
#   define WARNING(MESS);
#   define WARNINGA(MESS, LIST...);
#   define WARNINGU(MESS, LIST...);
#   define WARNINGG(MESS, LIST...);
#endif

#endif //__DXDEBUG_H__

