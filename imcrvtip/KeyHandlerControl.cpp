﻿
#include "imcrvtip.h"
#include "TextService.h"
#include "CandidateList.h"
#include "KeySender.h"

HRESULT CTextService::_HandleControl(TfEditCookie ec, ITfContext *pContext, BYTE sf, WCHAR ch)
{
	BOOL skipupdate = FALSE;

	switch(sf)
	{
	case SKK_KANA:
		if(abbrevmode && !showentry)
		{
			break;
		}

		switch(inputmode)
		{
		case im_hiragana:
		case im_katakana:
			if(inputkey && !showentry)
			{
				_ConvRoman();

				if(okuriidx != 0)
				{
					kana.erase(okuriidx, 1);
					okuriidx = 0;
				}

				//ひらがな/カタカナに変換
				_ConvKanaToKana(kana, inputmode, kana, (inputmode == im_hiragana ? im_katakana : im_hiragana));
				_HandleCharReturn(ec, pContext);
			}
			else
			{
				if(inputkey || !kana.empty() || !roman.empty())
				{
					_ConvRoman();
					_HandleCharReturn(ec, pContext);
				}

				//ひらがな/カタカナモードへ
				inputmode = (inputmode == im_hiragana ? im_katakana : im_hiragana);
				_UpdateLanguageBar();
			}
			return S_OK;
			break;
		case im_katakana_ank:
			if(inputkey || !kana.empty() || !roman.empty())
			{
				_ConvRoman();
				_HandleCharReturn(ec, pContext);
			}

			//ひらがなモードへ
			inputmode = im_hiragana;
			_UpdateLanguageBar();
			return S_OK;
			break;
		default:
			break;
		}
		break;

	case SKK_CONV_CHAR:
		if(abbrevmode && !showentry)
		{
			//全英に変換
			ASCII_JLATIN_CONV ajc;
			ajc.ascii[1] = L'\0';
			roman = kana;
			kana.clear();
			cursoridx = 0;
			for(size_t i = 0; i < roman.size(); i++)
			{
				ajc.ascii[0] = roman[i];
				if(_ConvAsciiJLatin(&ajc) == S_OK)
				{
					kana.append(ajc.jlatin);
				}
			}
			_HandleCharReturn(ec, pContext);
			return S_OK;
			break;
		}

		switch(inputmode)
		{
		case im_hiragana:
		case im_katakana:
			if(inputkey && !showentry)
			{
				_ConvRoman();

				if(okuriidx != 0)
				{
					kana.erase(okuriidx, 1);
					okuriidx = 0;
				}

				//半角ｶﾀｶﾅに変換
				_ConvKanaToKana(kana, inputmode, kana, im_katakana_ank);
				_HandleCharReturn(ec, pContext);
			}
			else
			{
				if(inputkey || !kana.empty() || !roman.empty())
				{
					_ConvRoman();
					_HandleCharReturn(ec, pContext);
				}

				//半角ｶﾀｶﾅモードへ
				inputmode = im_katakana_ank;
				_UpdateLanguageBar();
			}
			return S_OK;
			break;
		case im_katakana_ank:
			if(inputkey || !kana.empty() || !roman.empty())
			{
				_ConvRoman();
				_HandleCharReturn(ec, pContext);
			}

			//ひらがなモードへ
			inputmode = im_hiragana;
			_UpdateLanguageBar();
			return S_OK;
			break;
		default:
			break;
		}
		break;

	case SKK_JLATIN:
	case SKK_ASCII:
		if(abbrevmode && !showentry)
		{
			break;
		}

		switch(inputmode)
		{
		case im_hiragana:
		case im_katakana:
		case im_katakana_ank:
			if(inputkey || !kana.empty() || !roman.empty())
			{
				_ConvRoman();
				_HandleCharReturn(ec, pContext);
			}

			//アスキー/全英モードへ
			inputmode = (sf == SKK_ASCII ? im_ascii : im_jlatin);
			_UpdateLanguageBar();
			return S_OK;
			break;
		default:
			break;
		}
		break;

	case SKK_JMODE:
		switch(inputmode)
		{
		case im_jlatin:
		case im_ascii:
			//ひらがなモードへ
			inputmode = im_hiragana;
			_UpdateLanguageBar();
			return S_OK;
			break;
		default:
			break;
		}
		break;

	case SKK_ABBREV:
		if(abbrevmode && !showentry)
		{
			break;
		}

		switch(inputmode)
		{
		case im_hiragana:
		case im_katakana:
			_ConvRoman();

			if(!inputkey || showentry)
			{
				_HandleCharShift(ec, pContext);
				//見出し入力開始(abbrev)
				inputkey = TRUE;
				abbrevmode = TRUE;
			}
			_Update(ec, pContext);
			return S_OK;
			break;
		default:
			break;
		}
		break;

	case SKK_AFFIX:
		if(!inputkey || (abbrevmode && !showentry))
		{
			break;
		}

		if(showentry || (inputkey && kana.empty() && roman.empty()))
		{
			if(showentry)
			{
				_HandleCharShift(ec, pContext);
			}
			//見出し入力開始(接尾辞)
			inputkey = TRUE;
			ch = L'>';
			kana.push_back(ch);
			cursoridx++;
			_Update(ec, pContext);
			return S_OK;
		}

		switch(inputmode)
		{
		case im_hiragana:
		case im_katakana:
			_ConvRoman();

			if(!kana.empty() && okuriidx == 0)
			{
				ch = L'>';
				roman.clear();
				kana.push_back(ch);
				cursoridx = kana.size();
				if(cx_begincvokuri && !hintmode)
				{
					//辞書検索開始(接頭辞)
					showentry = TRUE;
					_StartConv(ec, pContext);
				}
			}
			_Update(ec, pContext);
			return S_OK;
			break;
		default:
			break;
		}
		break;

	case SKK_NEXT_CAND:
		if(showentry)
		{
			_NextConv();
			_Update(ec, pContext);
			return S_OK;
		}
		else if(inputkey)
		{
			_ConvRoman();

			if(okuriidx != 0 && okuriidx < kana.size())
			{
				if(kana[okuriidx] == CHAR_SKK_OKURI)
				{
					kana.erase(okuriidx, 1);
					if(okuriidx < cursoridx)
					{
						cursoridx--;
					}
					okuriidx = 0;
				}
			}

			if(!kana.empty())
			{
				//候補表示開始
				cursoridx = kana.size();
				showentry = TRUE;
				_StartConv(ec, pContext);
			}
			_Update(ec, pContext);
			return S_OK;
		}
		break;

	case SKK_PREV_CAND:
		if(showentry)
		{
			_PrevConv();

			if(showcandlist && (candidx < cx_untilcandlist - 1))
			{
				showcandlist = FALSE;
				if(pContext != NULL)
				{
					_EndCandidateList();
				}
			}

			if(!showentry && (cx_dynamiccomp || cx_dyncompmulti))
			{
				_DynamicComp(ec, pContext);
			}
			else
			{
				_Update(ec, pContext);
			}
			return S_OK;
		}
		break;

	case SKK_PURGE_DIC:
		if(showentry)
		{
			if(purgedicmode)
			{
				purgedicmode = FALSE;
				_DelUserDic((okuriidx == 0 ? REQ_USER_DEL_1 : REQ_USER_DEL_0),
					((candorgcnt <= candidx) ? searchkey : searchkeyorg),
					candidates[candidx].second.first);
				showentry = FALSE;
				candidx = 0;
				kana.clear();
				okuriidx = 0;
				cursoridx = 0;
				_HandleCharReturn(ec, pContext);
			}
			else
			{
				purgedicmode = TRUE;
				_Update(ec, pContext);
			}
			return S_OK;
		}
		break;

	case SKK_NEXT_COMP:
		if(inputkey && !showentry)
		{
			_ConvRoman();

			if(!complement)
			{
				_Update(ec, pContext);
			}

			_NextComp();

			BOOL vertical = _GetVertical(ec, pContext);

			if(complement && cx_compuserdic)
			{
				if(candidx == 0)
				{
					_UserDicComp();
				}

				if((!cx_stacompmulti && !cx_dyncompmulti) || !vertical || pContext == NULL)
				{
					okuriidx = kana.size();
					if(candidx < candidates.size() && !candidates[candidx].first.second.empty())
					{
						kana += markSP + candidates[candidx].first.second;
					}
					kana.insert(okuriidx, 1, CHAR_SKK_OKURI);

					_Update(ec, pContext);

					kana.erase(okuriidx);
					okuriidx = 0;
				}
			}
			else if(!complement && (cx_dynamiccomp || cx_dyncompmulti))
			{
				_DynamicComp(ec, pContext, TRUE);
			}
			else
			{
				if(!complement && cx_stacompmulti)
				{
					_EndCompletionList(ec, pContext);
				}

				if((!cx_stacompmulti && !cx_dyncompmulti) || !vertical || pContext == NULL)
				{
					_Update(ec, pContext);
				}
			}

			if(complement && candidx == 0 && pContext != NULL)
			{
				if(cx_dyncompmulti)
				{
					if(_pCandidateList == NULL)
					{
						showcandlist = FALSE;
						_ShowCandidateList(ec, pContext, FALSE, TRUE);
					}
					else
					{
						_pCandidateList->_UpdateComp();
					}
				}
				else if(cx_stacompmulti)
				{
					showcandlist = FALSE;
					_ShowCandidateList(ec, pContext, FALSE, TRUE);
				}
			}
			return S_OK;
		}
		break;

	case SKK_PREV_COMP:
		if(inputkey && !showentry)
		{
			_PrevComp();

			BOOL vertical = _GetVertical(ec, pContext);

			if(complement && cx_compuserdic)
			{
				if((!cx_stacompmulti && !cx_dyncompmulti) || !vertical || pContext == NULL)
				{
					okuriidx = kana.size();
					if(candidx < candidates.size() && !candidates[candidx].first.second.empty())
					{
						kana += markSP + candidates[candidx].first.second;
					}
					kana.insert(okuriidx, 1, CHAR_SKK_OKURI);

					_Update(ec, pContext);

					kana.erase(okuriidx);
					okuriidx = 0;
				}
			}
			else if(!complement && (cx_dynamiccomp || cx_dyncompmulti))
			{
				_DynamicComp(ec, pContext, TRUE);
			}
			else
			{
				if(!complement && cx_stacompmulti)
				{
					_EndCompletionList(ec, pContext);
				}

				if((!cx_stacompmulti && !cx_dyncompmulti) || !vertical || pContext == NULL)
				{
					_Update(ec, pContext);
				}
			}
			return S_OK;
		}
		break;

	case SKK_HINT:
		if(!inputkey || abbrevmode)
		{
			break;
		}

		if(showentry)
		{
			candidx = 0;
			showentry = FALSE;
		}

		_ConvRoman();

		if(!kana.empty() &&
			kana.find_first_of(CHAR_SKK_HINT) == std::wstring::npos)
		{
			hintmode = TRUE;
			cursoridx = kana.size();
			kana.insert(cursoridx, 1, CHAR_SKK_HINT);
			cursoridx++;
		}

		if(cx_dynamiccomp || cx_dyncompmulti)
		{
			_DynamicComp(ec, pContext);
		}
		else
		{
			_Update(ec, pContext);
		}
		return S_OK;
		break;

	case SKK_CONV_POINT:
		if(_HandleConvPoint(ec, pContext, ch) == S_OK)
		{
			return S_OK;
		}
		break;

	case SKK_DIRECT:
		if(inputkey && !showentry &&
			((okuriidx == 0) || ((okuriidx != 0) && (okuriidx + 1 != cursoridx))))
		{
			_ConvRoman();
			kana.insert(cursoridx, 1, ch);
			cursoridx++;

			if(cx_dynamiccomp || cx_dyncompmulti)
			{
				_DynamicComp(ec, pContext);
			}
			else
			{
				_Update(ec, pContext);
			}
			return S_OK;
		}
		break;

	case SKK_ENTER:
		if(showcandlist)
		{
			//_Update function needs showcandlist flag.
			if(pContext != NULL)
			{
				_EndCandidateList();
			}
		}

		if(inputkey || !kana.empty() || !roman.empty())
		{
			_ConvRoman();
			_HandleCharReturn(ec, pContext, (_GetSf(0, ch) == SKK_BACK ? TRUE : FALSE));
		}
		return S_OK;
		break;

	case SKK_CANCEL:
		if(showentry)
		{
			_ConvRoman();

			candidx = 0;
			showentry = FALSE;
			if(cx_delokuricncl && okuriidx != 0)
			{
				kana = kana.substr(0, okuriidx);
				okuriidx = 0;
				cursoridx = kana.size();
			}

			if(cx_delcvposcncl && okuriidx != 0)
			{
				kana.erase(okuriidx, 1);
				okuriidx = 0;
				cursoridx--;
			}

			if(showcandlist)
			{
				showcandlist = FALSE;
				if(pContext != NULL)
				{
					_EndCandidateList();
				}
			}

			if(cx_dynamiccomp || cx_dyncompmulti)
			{
				_DynamicComp(ec, pContext);
			}
			else
			{
				if(cx_stacompmulti)
				{
					_EndCompletionList(ec, pContext);
				}

				_Update(ec, pContext);
			}
		}
		else
		{
			if(inputkey || !kana.empty() || !roman.empty())
			{
				_ConvRoman();

				kana.clear();
				okuriidx = 0;
				cursoridx = 0;
				_HandleCharReturn(ec, pContext);
			}
		}
		return S_OK;
		break;

	case SKK_BACK:
		if(showentry)
		{
			if(_HandleControl(ec, pContext, (cx_backincenter ? SKK_ENTER : SKK_PREV_CAND), ch) == S_OK)
			{
				return S_OK;
			}
		}

		if(inputkey && roman.empty() && kana.empty())
		{
			_HandleCharReturn(ec, pContext);
			return S_OK;
		}

		if(!roman.empty())
		{
			roman.pop_back();
			if(!cx_showromancomp)
			{
				skipupdate = TRUE;
			}
			if(roman.empty())
			{
				if(okuriidx != 0 && okuriidx + 1 == cursoridx)
				{
					kana.erase(cursoridx - 1, 1);
					cursoridx--;
					okuriidx = 0;
					skipupdate = FALSE;
				}
			}
		}
		else if(okuriidx != 0 && okuriidx + 1 == cursoridx)
		{
			kana.erase(cursoridx - 1, 1);
			cursoridx--;
			okuriidx = 0;
		}
		else if(!kana.empty())
		{
			// surrogate pair
			if(cursoridx >= 2 &&
				IS_SURROGATE_PAIR(kana[cursoridx - 2], kana[cursoridx - 1]))
			{
				kana.erase(cursoridx - 2, 2);
				cursoridx -= 2;
				if(okuriidx != 0 && cursoridx < okuriidx)
				{
					okuriidx -= 2;
					if(okuriidx == 0)
					{
						kana.erase(0, 1);
					}
				}
			}
			else if(cursoridx >= 1)
			{
				kana.erase(cursoridx - 1, 1);
				cursoridx--;
				if(okuriidx != 0 && cursoridx < okuriidx)
				{
					okuriidx--;
					if(okuriidx == 0)
					{
						kana.erase(0, 1);
					}
				}
			}

			if(okuriidx != 0 && okuriidx + 1 == cursoridx)
			{
				kana.erase(cursoridx - 1, 1);
				cursoridx--;
				okuriidx = 0;
			}
		}

		if(!inputkey && roman.empty() && kana.empty())
		{
			_HandleCharReturn(ec, pContext);
		}
		else if(!skipupdate)
		{
			if(cx_dynamiccomp || cx_dyncompmulti)
			{
				_DynamicComp(ec, pContext);
			}
			else
			{
				_Update(ec, pContext);
			}
		}
		return S_OK;
		break;

	case SKK_DELETE:
		if(!inputkey || showentry || kana.empty())
		{
			break;
		}

		if(okuriidx != 0 && okuriidx == cursoridx)
		{
			kana.erase(cursoridx, 1);
			okuriidx = 0;
		}

		// surrogate pair
		if(kana.size() - cursoridx >= 2 &&
			IS_SURROGATE_PAIR(kana[cursoridx], kana[cursoridx + 1]))
		{
			kana.erase(cursoridx, 2);
			if(okuriidx >= 2 && cursoridx < okuriidx)
			{
				okuriidx -= 2;
				if(okuriidx == 0)
				{
					kana.erase(cursoridx, 1);
				}
			}
		}
		else
		{
			kana.erase(cursoridx, 1);
			if(okuriidx >= 1 && cursoridx < okuriidx)
			{
				okuriidx--;
				if(okuriidx == 0)
				{
					kana.erase(cursoridx, 1);
				}
			}
		}

		if(cx_dynamiccomp || cx_dyncompmulti)
		{
			_DynamicComp(ec, pContext);
		}
		else
		{
			_Update(ec, pContext);
		}
		return S_OK;
		break;

	case SKK_VOID:
		return S_OK;
		break;

	case SKK_LEFT:
		if(showentry)
		{
			break;
		}

		if(!roman.empty() || (okuriidx != 0 && okuriidx + 1 == cursoridx))
		{
			_ConvRoman();

			if(inputkey)
			{
				_HandleCharShift(ec, pContext);
			}
			else
			{
				_HandleCharReturn(ec, pContext);
				return S_OK;
			}
		}
		else if(!kana.empty() && cursoridx > 0)
		{
			// surrogate pair
			if(cursoridx >= 2 &&
				IS_SURROGATE_PAIR(kana[cursoridx - 2], kana[cursoridx - 1]))
			{
				cursoridx -= 2;
			}
			else
			{
				cursoridx--;
			}

			if(okuriidx != 0 && okuriidx + 1 == cursoridx)
			{
				cursoridx--;
			}
		}

		if(cx_dynamiccomp || cx_dyncompmulti)
		{
			_DynamicComp(ec, pContext);
		}
		else
		{
			_Update(ec, pContext);
		}
		return S_OK;
		break;

	case SKK_UP:
		if(showentry)
		{
			break;
		}

		if(!roman.empty() || (okuriidx != 0 && okuriidx + 1 == cursoridx))
		{
			_ConvRoman();

			if(inputkey)
			{
				_HandleCharShift(ec, pContext);
			}
			else
			{
				_HandleCharReturn(ec, pContext);
				return S_OK;
			}
		}
		else
		{
			cursoridx = 0;
		}

		if(cx_dynamiccomp || cx_dyncompmulti)
		{
			_DynamicComp(ec, pContext);
		}
		else
		{
			_Update(ec, pContext);
		}
		return S_OK;
		break;

	case SKK_RIGHT:
		if(showentry)
		{
			break;
		}

		if(!roman.empty() || (okuriidx != 0 && okuriidx + 1 == cursoridx))
		{
			_ConvRoman();

			if(inputkey)
			{
				_HandleCharShift(ec, pContext);
			}
			else
			{
				_HandleCharReturn(ec, pContext);
				return S_OK;
			}
		}
		else if(!kana.empty() && cursoridx < kana.size())
		{
			// surrogate pair
			if(kana.size() - cursoridx >= 2 &&
				IS_SURROGATE_PAIR(kana[cursoridx], kana[cursoridx + 1]))
			{
				cursoridx += 2;
			}
			else
			{
				cursoridx++;
			}

			if(okuriidx != 0 && okuriidx + 1 == cursoridx)
			{
				cursoridx++;
			}
		}

		if(cx_dynamiccomp || cx_dyncompmulti)
		{
			_DynamicComp(ec, pContext);
		}
		else
		{
			_Update(ec, pContext);
		}
		return S_OK;
		break;

	case SKK_DOWN:
		if(showentry)
		{
			break;
		}

		if(!roman.empty() || (okuriidx != 0 && okuriidx + 1 == cursoridx))
		{
			_ConvRoman();

			if(inputkey)
			{
				_HandleCharShift(ec, pContext);
			}
			else
			{
				_HandleCharReturn(ec, pContext);
				return S_OK;
			}
		}
		else
		{
			cursoridx = kana.size();
		}

		if(cx_dynamiccomp || cx_dyncompmulti)
		{
			_DynamicComp(ec, pContext);
		}
		else
		{
			_Update(ec, pContext);
		}
		return S_OK;
		break;

	case SKK_PASTE:
		if(!inputkey || showentry)
		{
			break;
		}

		_ConvRoman();

		if(IsClipboardFormatAvailable(CF_UNICODETEXT))
		{
			if(OpenClipboard(NULL))
			{
				HANDLE hCB = GetClipboardData(CF_UNICODETEXT);
				if(hCB != NULL)
				{
					PWCHAR pwCB = (PWCHAR)GlobalLock(hCB);
					if(pwCB != NULL)
					{
						std::wstring scb = std::regex_replace(std::wstring(pwCB),
							std::wregex(L"[\\x00-\\x19]"), std::wstring(L""));
						kana.insert(cursoridx, scb);
						if(okuriidx != 0 && cursoridx <= okuriidx)
						{
							okuriidx += scb.size();
						}
						cursoridx += scb.size();
						if(cx_dynamiccomp || cx_dyncompmulti)
						{
							_DynamicComp(ec, pContext);
						}
						else
						{
							_Update(ec, pContext);
						}
						GlobalUnlock(hCB);
					}
				}
				CloseClipboard();
			}
		}
		break;

	case SKK_OTHERIME:
		_ConvRoman();
		if(_ShowInputMode)
		{
			_HandleCharShift(ec, pContext);
		}
		else
		{
			_HandleCharReturn(ec, pContext);
		}
		_ClearComposition();
		postbuf.clear();
		_SetKeyboardOpen(FALSE);

		KeySender::OtherIme();
		return S_OK;
		break;

	//DeleterがBS送り付けによりカーソル直前文字列を削除した後に実行される。
	//+ 削除した文字列を置換する文字列を確定する。
	//+ pending.mazeの場合、交ぜ書き変換の候補表示を開始する。
	//(TSFによるカーソル直前文字列削除ができなかった場合用。)
	case SKK_AFTER_DELETER:
		{
			mozc::commands::Output pending;
			pending.CopyFrom(deleter.pending_output());
			if(pending.maze)
			{
				_StartConvWithYomi(ec, pContext, pending.kana);
			}
			else
			{
				kana = pending.kana;
				cursoridx = kana.size();
				_HandleCharReturn(ec, pContext);
			}
		}
		return S_OK;
		break;

	default:
		break;
	}

	return E_PENDING;
}

HRESULT CTextService::_HandleConvPoint(TfEditCookie ec, ITfContext *pContext, WCHAR ch)
{
	if(abbrevmode && !showentry)
	{
		return E_PENDING;
	}

	switch(inputmode)
	{
	case im_hiragana:
	case im_katakana:
		if(showentry)
		{
			_HandleCharShift(ec, pContext);
		}

		if(!inputkey)
		{
			if(_ConvShift(ch))
			{
				if(!kana.empty())
				{
					_HandleCharShift(ec, pContext);
				}
				//見出し入力開始
				inputkey = TRUE;
				_Update(ec, pContext);
			}
		}
		else
		{
			if(_ConvShift(ch) && (okuriidx == 0) && (cursoridx != 0))
			{
				//送り仮名入力開始
				kana.insert(cursoridx, 1, CHAR_SKK_OKURI);	//送りローマ字
				okuriidx = cursoridx;
				cursoridx++;
				if(cx_dynamiccomp || cx_dyncompmulti)
				{
					_DynamicComp(ec, pContext);
				}
				else
				{
					_Update(ec, pContext);
				}
			}
		}

		if(ch == L'\0')
		{
			return S_OK;
		}
		break;
	default:
		break;
	}

	return E_PENDING;
}
