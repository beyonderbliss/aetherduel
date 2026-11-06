#include "game_config.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <irrlicht.h>
#include "config.h"
#include "deck_con.h"
#include "utils.h"
#include "data_manager.h"
#include "deck_manager.h"
#include "image_manager.h"
#include "game.h"
#include "porting.h"
#include "duelclient.h"
#include "single_mode.h"
#include "client_card.h"
#include "fmt.h"

namespace ygo {

static int parse_filter(const wchar_t* pstr, uint32_t& type) {
	if(*pstr == L'=') {
		type = 1;
		return BufferIO::GetVal(pstr + 1);
	}
	if(*pstr >= L'0' && *pstr <= L'9') {
		type = 1;
		return BufferIO::GetVal(pstr);
	}
	if(*pstr == L'>') {
		if(*(pstr + 1) == L'=') {
			type = 2;
			return BufferIO::GetVal(pstr + 2);
		}
		type = 3;
		return BufferIO::GetVal(pstr + 1);
	}
	if(*pstr == L'<') {
		if(*(pstr + 1) == L'=') {
			type = 4;
			return BufferIO::GetVal(pstr + 2);
		}
		type = 5;
		return BufferIO::GetVal(pstr + 1);
	}
	if(*pstr == L'?') {
		type = 6;
		return 0;
	}
	type = 0;
	return 0;
}

void DeckBuilder::StartDeckNameInput(int mode) {
	if(mode == 2) {
		const int selected = mainGame->cbDBDecks->getSelected();
		if(selected < 0)
			return;
		mainGame->ebDeckname->setText(mainGame->cbDBDecks->getItem(selected));
	} else {
		mainGame->ebDeckname->setText(L"");
	}
	deck_name_input_mode = mode;
	mainGame->UpdateDeckMenuLayout();
	mainGame->env->setFocus(mainGame->ebDeckname);
	if(mode == 2) {
		irr::SEvent cursorEvent{};
		cursorEvent.EventType = irr::EET_KEY_INPUT_EVENT;
		cursorEvent.KeyInput.Key = irr::KEY_END;
		cursorEvent.KeyInput.PressedDown = true;
		mainGame->ebDeckname->OnEvent(cursorEvent);
	}
#if EDOPRO_ANDROID
	porting::setDeckNameInputMode(true);
	porting::showInputDialog(BufferIO::EncodeUTF8(mainGame->ebDeckname->getText()), true);
#endif
}

void DeckBuilder::CancelDeckNameInput() {
	if(deck_name_input_mode == 0)
		return;
#if EDOPRO_ANDROID
	porting::setDeckNameInputMode(false);
#endif
	deck_name_input_mode = 0;
	mainGame->ebDeckname->setText(L"");
	mainGame->UpdateDeckMenuLayout();
	mainGame->env->setFocus(mainGame->btnRenameDeck);
}

void DeckBuilder::CommitDeckNameInput() {
	if(deck_name_input_mode == 0)
		return;
	const std::wstring deckName = mainGame->ebDeckname->getText();
	if(deckName.empty()) {
#if EDOPRO_ANDROID
		mainGame->env->setFocus(mainGame->ebDeckname);
		porting::showInputDialog(BufferIO::EncodeUTF8(mainGame->ebDeckname->getText()), true);
#endif
		return;
	}
	const int current = mainGame->cbDBDecks->getSelected();
	int duplicate = -1;
	const auto upper = Utils::ToUpperNoAccents(epro::wstringview(deckName));
	for(irr::u32 i = 0; i < mainGame->cbDBDecks->getItemCount(); ++i) {
		if(Utils::EqualIgnoreCaseFirst<epro::wstringview>(upper, mainGame->cbDBDecks->getItem(i))) {
			duplicate = static_cast<int>(i);
			break;
		}
	}
	if(duplicate >= 0 && (deck_name_input_mode != 2 || duplicate != current)) {
		mainGame->stACMessage->setText(gDataManager->GetSysString(1339).data());
		mainGame->PopupElement(mainGame->wACMessage, 40);
#if EDOPRO_ANDROID
		mainGame->env->setFocus(mainGame->ebDeckname);
		porting::showInputDialog(BufferIO::EncodeUTF8(mainGame->ebDeckname->getText()), true);
#endif
		return;
	}
	if(deck_name_input_mode == 1) {
		if(DeckManager::SaveDeck(Utils::ToPathString(deckName), current_deck)) {
			mainGame->cbDBDecks->setSelected(mainGame->cbDBDecks->addItem(deckName.c_str()));
			CancelDeckNameInput();
			mainGame->stACMessage->setText(gDataManager->GetSysString(1335).data());
			mainGame->PopupElement(mainGame->wACMessage, 20);
		} else {
			mainGame->stACMessage->setText(gDataManager->GetSysString(1364).data());
			mainGame->PopupElement(mainGame->wACMessage, 30);
#if EDOPRO_ANDROID
			mainGame->env->setFocus(mainGame->ebDeckname);
		porting::showInputDialog(BufferIO::EncodeUTF8(mainGame->ebDeckname->getText()), true);
#endif
		}
	} else if(current < 0) {
		CancelDeckNameInput();
	} else if(wcscmp(deckName.c_str(), mainGame->cbDBDecks->getItem(current)) == 0) {
		CancelDeckNameInput();
	} else if(DeckManager::RenameDeck(Utils::ToPathString(mainGame->cbDBDecks->getItem(current)), Utils::ToPathString(deckName))) {
		mainGame->cbDBDecks->removeItem(current);
		mainGame->cbDBDecks->setSelected(mainGame->cbDBDecks->addItem(deckName.c_str()));
		CancelDeckNameInput();
	} else {
		mainGame->stACMessage->setText(gDataManager->GetSysString(1364).data());
		mainGame->PopupElement(mainGame->wACMessage, 30);
#if EDOPRO_ANDROID
		mainGame->env->setFocus(mainGame->ebDeckname);
		porting::showInputDialog(BufferIO::EncodeUTF8(mainGame->ebDeckname->getText()), true);
#endif
	}
}

void DeckBuilder::Initialize(bool refresh) {
	mainGame->is_building = true;
	mainGame->is_siding = false;
	active_deck_tab = 0;
	if(refresh)
		mainGame->ClearCardInfo();
	mainGame->mTopMenu->setVisible(false);
	mainGame->wInfos->setVisible(true);
	mainGame->wCardImg->setVisible(true);
	mainGame->wDeckEdit->setVisible(true);
	deck_name_input_mode = 0;
	mainGame->UpdateDeckMenuLayout();
	mainGame->wFilter->setVisible(true);
	mainGame->wSort->setVisible(true);
	mainGame->SetVerticalButtonVisible(mainGame->btnLeaveGame, true);
	mainGame->SetVerticalButtonVisible(mainGame->btnSettingsMenu, true);
	mainGame->btnLeaveGame->setText(gDataManager->GetSysString(1306).data());
	mainGame->btnSideOK->setVisible(false);
	mainGame->btnSideShuffle->setVisible(false);
	mainGame->btnSideSort->setVisible(false);
	mainGame->btnSideReload->setVisible(false);
	mainGame->SetVerticalButtonVisible(mainGame->btnHandTest, true);
	mainGame->SetVerticalButtonVisible(mainGame->btnHandTestSettings, true);
	mainGame->SetVerticalButtonVisible(mainGame->btnYdkeManage, true);
	filterList = &gdeckManager->_lfList[mainGame->cbDBLFList->getSelected()];
	if(refresh) {
		ClearSearch();
	} else if(results.size()) {
		const float oldScroll = search_result_scroll_offset;
		StartFilter();
		const float maxScroll = std::max(0.0f, static_cast<float>(results.size()) * kDeckSearchResultRowHeight - kDeckSearchResultViewportHeight);
		search_result_scroll_offset = std::clamp(oldScroll, 0.0f, maxScroll);
	}
	mouse_pos.set(0, 0);
	hovered_code = 0;
	hovered_pos = 0;
	hovered_seq = -1;
	is_lastcard = 0;
	is_draging = false;
	drag_pending = false;
	drag_start_seq = -1;
	search_result_scroll_velocity = 0.0f;
	search_result_scroll_inertia = false;
	search_result_scroll_pending = false;
	search_result_scroll_active = false;
	deck_scroll_pending = false;
	deck_scroll_active = false;
	deck_scroll_free = false;
	main_deck_scroll_offset = 0.0f;
	main_deck_scroll_velocity = 0.0f;
	main_deck_scroll_inertia = false;
	drag_source_in_deck = false;
	prev_deck = mainGame->cbDBDecks->getSelected();
	prev_operation = 0;
	mainGame->SetMessageWindow();
	mainGame->UpdateVerticalButtonLayout();
	mainGame->device->setEventReceiver(this);
}
void DeckBuilder::Terminate(bool showmenu) {
	mainGame->is_building = false;
	if(mainGame->cardViewFullscreen) {
		mainGame->cardViewFullscreen = false;
		mainGame->ResizeCardinfoWindow(gGameConfig->keep_cardinfo_aspect_ratio);
	}
	mainGame->is_siding = false;
	if(showmenu) {
		mainGame->ClearCardInfo();
		mainGame->mTopMenu->setVisible(true);
	}
	mainGame->wDeckEdit->setVisible(false);
	mainGame->wCategories->setVisible(false);
	mainGame->wFilter->setVisible(false);
	mainGame->wSort->setVisible(false);
	if(showmenu) {
		mainGame->wCardImg->setVisible(false);
		mainGame->wInfos->setVisible(false);
		mainGame->SetVerticalButtonVisible(mainGame->btnLeaveGame, false);
		mainGame->SetVerticalButtonVisible(mainGame->btnSettingsMenu, false);
		mainGame->PopupElement(mainGame->wMainMenu);
		mainGame->ClearTextures();
		mainGame->ClearCardInfo(0);
		gdeckManager->ClearDummies();
	}
	mainGame->SetVerticalButtonVisible(mainGame->btnHandTest, false);
	mainGame->SetVerticalButtonVisible(mainGame->btnHandTestSettings, false);
	mainGame->SetVerticalButtonVisible(mainGame->btnYdkeManage, false);
	mainGame->wYdkeManage->setVisible(false);
	mainGame->wHandTest->setVisible(false);
	mainGame->device->setEventReceiver(&mainGame->menuHandler);
	mainGame->wACMessage->setVisible(false);
	mainGame->SetMessageWindow();
	mainGame->UpdateVerticalButtonLayout();
	int sel = mainGame->cbDBDecks->getSelected();
	if(sel >= 0)
		gGameConfig->lastdeck = mainGame->cbDBDecks->getItem(sel);
	gGameConfig->lastlflist = gdeckManager->_lfList[mainGame->cbDBLFList->getSelected()].hash;
}
bool DeckBuilder::SetCurrentDeckFromFile(epro::path_stringview file, bool separated, RITUAL_LOCATION rituals_in_extra) {
	Deck tmp;
	if(!DeckManager::LoadDeckFromFile(file, tmp, separated, rituals_in_extra))
		return false;
	SetCurrentDeck(std::move(tmp));
	return true;
}
void DeckBuilder::ImportDeck() {
	const wchar_t* deck_string = Utils::OSOperator->getTextFromClipboard();
	if(deck_string) {
		epro::wstringview text{ deck_string };
		if(starts_with(text, L"ydke://"))
			DeckManager::ImportDeckYdke(current_deck, text.data());
		else
			(void)DeckManager::ImportDeckBase64Omega(current_deck, text);
		RefreshLimitationStatus();
	}
}
void DeckBuilder::ExportDeckToClipboard(bool plain_text) {
	auto deck_string = plain_text ? DeckManager::ExportDeckCardNames(current_deck) : DeckManager::ExportDeckYdke(current_deck);
	if(!deck_string.empty()) {
		Utils::OSOperator->copyToClipboard(deck_string.data());
		mainGame->stACMessage->setText(gDataManager->GetSysString(1368).data());
	} else {
		mainGame->stACMessage->setText(gDataManager->GetSysString(1369).data());
	}
	mainGame->PopupElement(mainGame->wACMessage, 20);
}
bool DeckBuilder::OnEvent(const irr::SEvent& event) {
	const auto getSearchResultMaxScroll = [this]() {
		return std::max(0.0f, static_cast<float>(results.size()) * kDeckSearchResultRowHeight - kDeckSearchResultViewportHeight);
	};
	const auto updateSearchResultFreeScroll = [this, &getSearchResultMaxScroll](int logicalY) {
		const float deltaY = static_cast<float>(logicalY - search_result_scroll_start_y);
		search_result_scroll_offset = std::clamp(search_result_scroll_start_offset - deltaY, 0.0f, getSearchResultMaxScroll());
	};
	const auto getMainDeckMaxScroll = [this]() {
		const int unlockedRows = std::max(3, static_cast<int>(current_deck.main.size() / 5) + 1);
		return static_cast<float>(unlockedRows - 3);
	};
	const auto updateMainDeckFreeScroll = [this, &getMainDeckMaxScroll](int logicalY) {
		const auto grid = mainGame->GetDeckGridLayout();
		const float deltaRows = static_cast<float>(logicalY - deck_scroll_start_y) /
			static_cast<float>(std::max(1, grid.rowStep));
		main_deck_scroll_offset = std::clamp(
			deck_scroll_start_offset - deltaRows, 0.0f, getMainDeckMaxScroll());
	};
	// The DeckBuilder replaces ClientField as the active event receiver.
	// Keep the shared card viewer interactive in both editor and duel modes.
	if(event.EventType == irr::EET_MOUSE_INPUT_EVENT &&
	   event.MouseInput.Event == irr::EMIE_LMOUSE_LEFT_UP &&
	   mainGame->wCardImg->isVisible() && mainGame->imgCard->isVisible() &&
	   !is_draging && !drag_pending &&
	   !deck_scroll_pending && !deck_scroll_active) {
		const irr::core::vector2di tap(event.MouseInput.X, event.MouseInput.Y);
		if(mainGame->imgCard->getAbsolutePosition().isPointInside(tap)) {
			mainGame->cardViewFullscreen = !mainGame->cardViewFullscreen;
			mainGame->ResizeCardinfoWindow(gGameConfig->keep_cardinfo_aspect_ratio);
			mainGame->UpdateDeckMenuLayout();
			return true;
		}
	}
	if(event.EventType == irr::EET_MOUSE_INPUT_EVENT &&
	   event.MouseInput.Event == irr::EMIE_LMOUSE_LEFT_UP && !is_draging &&
	   !deck_scroll_pending && !deck_scroll_active &&
	   !mainGame->wCategories->isVisible() && !mainGame->wQuery->isVisible()) {
		const auto pos = mainGame->Resize(event.MouseInput.X, event.MouseInput.Y, true);
		const int panelLeft = mainGame->GetDeckPanelLeft();
		if(pos.X >= panelLeft && pos.X < 800 && pos.Y >= 140 && pos.Y <= 172) {
			const int tabWidth = (800 - panelLeft) / 3;
			active_deck_tab = std::min(2, (pos.X - panelLeft) / std::max(1, tabWidth));
			hovered_code = 0;
			hovered_pos = 0;
			hovered_seq = -1;
			return true;
		}
	}
	bool stopPropagation = false;
	if(mainGame->dField.OnCommonEvent(event, stopPropagation))
		return stopPropagation;
	switch(event.EventType) {
	case irr::EET_GUI_EVENT: {
		int id = event.GUIEvent.Caller->getID();
		if(mainGame->wCategories->isVisible() && id != BUTTON_CATEGORY_OK)
			break;
		if(mainGame->wQuery->isVisible() && id != BUTTON_YES && id != BUTTON_NO)
			break;
		if(mainGame->wLinkMarks->isVisible() && id != BUTTON_MARKERS_OK)
			break;
		switch(event.GUIEvent.EventType) {
		case irr::gui::EGET_BUTTON_CLICKED: {
			switch(id) {
			case BUTTON_HAND_TEST_SETTINGS: {
				mainGame->PopupElement(mainGame->wHandTest);
				break;
			}
			case BUTTON_HAND_TEST:
			case BUTTON_HAND_TEST_START: {
				Terminate(false);
				SingleMode::DuelOptions options("hand-test-mode");
				options.handTestNoOpponent = mainGame->chkHandTestNoOpponent->isChecked();
				try {
					options.startingDrawCount = std::stoi(mainGame->ebHandTestStartHand->getText());
				} catch(...) {}
#define CHECK(MR) case (MR - 1):{  options.duelFlags = DUEL_MODE_MR##MR; break; }
				switch (mainGame->cbHandTestDuelRule->getSelected()) {
				CHECK(1)
				CHECK(2)
				CHECK(3)
				CHECK(4)
				CHECK(5)
				case 5: {
					options.duelFlags = DUEL_MODE_SPEED;
					break;
				}
				case 6: {
					options.duelFlags = DUEL_MODE_RUSH;
					break;
				}
				case 7: {
					options.duelFlags = DUEL_MODE_GOAT;
					break;
				}
				}
#undef CHECK
				options.duelFlags |= mainGame->chkHandTestNoShuffle->isChecked() ? DUEL_PSEUDO_SHUFFLE : 0;
				SingleMode::singleSignal.SetNoWait(false);
				SingleMode::StartPlay(std::move(options));
				break;
			}
			case BUTTON_HAND_TEST_CANCEL: {
				mainGame->HideElement(mainGame->wHandTest);
				mainGame->env->setFocus(mainGame->btnHandTestSettings);
				break;
			}
			case BUTTON_DECK_YDKE_MANAGE: {
				mainGame->PopupElement(mainGame->wYdkeManage);
				break;
			}
			case BUTTON_IMPORT_YDKE: {
				ImportDeck();
				break;
			}
			case BUTTON_EXPORT_YDKE: {
				ExportDeckToClipboard(false);
				break;
			}
			case BUTTON_EXPORT_DECK_PLAINTEXT: {
				ExportDeckToClipboard(true);
				break;
			}
			case BUTTON_CLOSE_YDKE_WINDOW: {
				mainGame->HideElement(mainGame->wYdkeManage);
				mainGame->env->setFocus(mainGame->btnYdkeManage);
				break;
			}
			case BUTTON_CLEAR_DECK: {
				if(gGameConfig->confirm_clear_deck) {
					std::lock_guard<epro::mutex> lock(mainGame->gMutex);
					mainGame->stQMessage->setText(epro::format(L"{}", gDataManager->GetSysString(2004)).data());
					mainGame->PopupElement(mainGame->wQuery);
					prev_operation = id;
					break;
				}
				ClearDeck();
				break;
			}
			case BUTTON_SORT_DECK: {
				std::sort(current_deck.main.begin(), current_deck.main.end(), DataManager::deck_sort_lv);
				std::sort(current_deck.extra.begin(), current_deck.extra.end(), DataManager::deck_sort_lv);
				std::sort(current_deck.side.begin(), current_deck.side.end(), DataManager::deck_sort_lv);
				break;
			}
			case BUTTON_SHUFFLE_DECK: {
				std::shuffle(
					current_deck.main.begin(),
					current_deck.main.end(),
					Utils::GetRandomNumberGenerator()
				);
				break;
			}
			case BUTTON_SAVE_DECK: {
				int sel = mainGame->cbDBDecks->getSelected();
				if(sel >= 0 && DeckManager::SaveDeck(Utils::ToPathString(mainGame->cbDBDecks->getItem(sel)), current_deck)) {
					mainGame->stACMessage->setText(gDataManager->GetSysString(1335).data());
					mainGame->PopupElement(mainGame->wACMessage, 20);
				}
				break;
			}
			case BUTTON_SAVE_DECK_AS: {
				if(deck_name_input_mode == 0)
					StartDeckNameInput(1);
				else
					CommitDeckNameInput();
				break;
			}
			case BUTTON_DELETE_DECK: {
				int sel = mainGame->cbDBDecks->getSelected();
				if(sel == -1)
					break;
				std::lock_guard<epro::mutex> lock(mainGame->gMutex);
				mainGame->stQMessage->setText(epro::format(L"{}\n{}", mainGame->cbDBDecks->getItem(sel), gDataManager->GetSysString(1337)).data());
				mainGame->PopupElement(mainGame->wQuery);
				prev_operation = id;
				break;
			}
			case BUTTON_RENAME_DECK: {
				if(deck_name_input_mode != 0)
					CancelDeckNameInput();
				else
					StartDeckNameInput(2);
				break;
			}
			case BUTTON_LEAVE_GAME: {
				Terminate();
				break;
			}
			case BUTTON_EFFECT_FILTER: {
				mainGame->PopupElement(mainGame->wCategories);
				break;
			}
			case BUTTON_START_FILTER: {
				StartFilter();
				break;
			}
			case BUTTON_CLEAR_FILTER: {
				ClearSearch();
				break;
			}
			case BUTTON_CATEGORY_OK: {
				filter_effect = 0;
				long long filter = 0x1;
				for(int i = 0; i < 32; ++i, filter <<= 1)
					if(mainGame->chkCategory[i]->isChecked())
						filter_effect |= filter;
				mainGame->HideElement(mainGame->wCategories);
				break;
			}
			case BUTTON_SIDE_OK: {
				auto old_skills = DeckManager::TypeCount(gdeckManager->pre_deck.main, TYPE_SKILL);
				auto new_skills = DeckManager::TypeCount(current_deck.main, TYPE_SKILL);
				if((current_deck.main.size() - new_skills) != (gdeckManager->pre_deck.main.size() - old_skills)
				   || current_deck.extra.size() != gdeckManager->pre_deck.extra.size()) {
					mainGame->PopupMessage(gDataManager->GetSysString(1408));
					break;
				}
				mainGame->ClearCardInfo();
				const auto& deck = current_deck;
				uint8_t deckbuf[0xf000];
				auto* pdeck = deckbuf;
				static constexpr auto max_deck_size = sizeof(deckbuf) / sizeof(uint32_t) - 2;
				const auto totsize = deck.main.size() + deck.extra.size() + deck.side.size();
				if(totsize > max_deck_size) {
					mainGame->PopupMessage(gDataManager->GetSysString(1410));
					break;
				}
				BufferIO::Write<uint32_t>(pdeck, static_cast<uint32_t>(deck.main.size() + deck.extra.size()));
				BufferIO::Write<uint32_t>(pdeck, static_cast<uint32_t>(deck.side.size()));
				for(const auto& pcard : deck.main)
					BufferIO::Write<uint32_t>(pdeck, pcard->code);
				for(const auto& pcard : deck.extra)
					BufferIO::Write<uint32_t>(pdeck, pcard->code);
				for(const auto& pcard : deck.side)
					BufferIO::Write<uint32_t>(pdeck, pcard->code);
				DuelClient::SendBufferToServer(CTOS_UPDATE_DECK, deckbuf, pdeck - deckbuf);
				gdeckManager->sent_deck = current_deck;
				break;
			}
			case BUTTON_SIDE_RELOAD: {
				current_deck = gdeckManager->pre_deck;
				RefreshLimitationStatus();
				break;
			}
			case BUTTON_MSG_OK: {
				mainGame->HideElement(mainGame->wMessage);
				mainGame->actionSignal.Set();
				break;
			}
			case BUTTON_YES: {
				mainGame->HideElement(mainGame->wQuery);
				if(!mainGame->is_building || mainGame->is_siding)
					break;
				switch(prev_operation) {
				case BUTTON_DELETE_DECK : {
					int sel = mainGame->cbDBDecks->getSelected();
					if(DeckManager::DeleteDeck(current_deck, Utils::ToPathString(mainGame->cbDBDecks->getItem(sel)))) {
						mainGame->cbDBDecks->removeItem(sel);
						int count = mainGame->cbDBDecks->getItemCount();
						if(sel >= count)
							sel = count - 1;
						mainGame->cbDBDecks->setSelected(sel);
						if(sel != -1)
							mainGame->deckBuilder.SetCurrentDeckFromFile(Utils::ToPathString(mainGame->cbDBDecks->getItem(sel)), true);
						mainGame->stACMessage->setText(gDataManager->GetSysString(1338).data());
						mainGame->PopupElement(mainGame->wACMessage, 20);
						prev_deck = sel;
					}
					break;
				}
				case BUTTON_LEAVE_GAME: {
					Terminate();
					break;
				}
				case COMBOBOX_DBDECKS: {
					int sel = mainGame->cbDBDecks->getSelected();
					mainGame->deckBuilder.SetCurrentDeckFromFile(Utils::ToPathString(mainGame->cbDBDecks->getItem(sel)), true);
					prev_deck = sel;
					break;
				}
				case BUTTON_CLEAR_DECK: {
					ClearDeck();
					break;
				}
				}
				prev_operation = 0;
				break;
			}
			case BUTTON_NO: {
				mainGame->HideElement(mainGame->wQuery);
				if (prev_operation == COMBOBOX_DBDECKS) {
					mainGame->cbDBDecks->setSelected(prev_deck);
				}
				prev_operation = 0;
				break;
			}
			case BUTTON_MARKS_FILTER: {
				mainGame->PopupElement(mainGame->wLinkMarks);
				break;
			}
			case BUTTON_MARKERS_OK: {
				filter_marks = 0;
				if (mainGame->btnMark[0]->isPressed())
					filter_marks |= 0100;
				if (mainGame->btnMark[1]->isPressed())
					filter_marks |= 0200;
				if (mainGame->btnMark[2]->isPressed())
					filter_marks |= 0400;
				if (mainGame->btnMark[3]->isPressed())
					filter_marks |= 0010;
				if (mainGame->btnMark[4]->isPressed())
					filter_marks |= 0040;
				if (mainGame->btnMark[5]->isPressed())
					filter_marks |= 0001;
				if (mainGame->btnMark[6]->isPressed())
					filter_marks |= 0002;
				if (mainGame->btnMark[7]->isPressed())
					filter_marks |= 0004;
				mainGame->HideElement(mainGame->wLinkMarks);
				StartFilter(true);
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_EDITBOX_ENTER: {
			switch(id) {
			case EDITBOX_ATTACK:
			case EDITBOX_DEFENSE:
			case EDITBOX_STAR:
			case EDITBOX_SCALE:
			case EDITBOX_KEYWORD: {
				StartFilter();
				break;
			}
			case EDITBOX_DECK_NAME: {
				CommitDeckNameInput();
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_EDITBOX_CHANGED: {
			auto caller = event.GUIEvent.Caller;
			auto StartFilterIfLongerThan = [&](size_t length) {
				std::wstring filter = caller->getText();
				if (filter.size() > length)
					StartFilter();
			};
			switch (id) {
			case EDITBOX_ATTACK:
			case EDITBOX_DEFENSE:
			case EDITBOX_KEYWORD: {
				StartFilterIfLongerThan(2);
				break;
			}
			case EDITBOX_STAR:
			case EDITBOX_SCALE: {
				StartFilter();
				break;
			}
			case EDITBOX_DECK_NAME: {
				mainGame->ValidateName(caller);
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_COMBO_BOX_CHANGED: {
			switch(id) {
			case COMBOBOX_DBLFLIST: {
				filterList = &gdeckManager->_lfList[mainGame->cbDBLFList->getSelected()];
				mainGame->ReloadCBLimit();
				StartFilter(true);
				break;
			}
			case COMBOBOX_DBDECKS: {
				int sel = mainGame->cbDBDecks->getSelected();
				if(sel >= 0)
					mainGame->deckBuilder.SetCurrentDeckFromFile(Utils::ToPathString(mainGame->cbDBDecks->getItem(sel)), true);
				prev_deck = sel;
				break;
			}
			case COMBOBOX_MAINTYPE: {
				mainGame->ReloadCBCardType2();
				mainGame->cbAttribute->setSelected(0);
				mainGame->cbRace->setSelected(0);
				mainGame->ebAttack->setText(L"");
				mainGame->ebDefense->setText(L"");
				mainGame->ebStar->setText(L"");
				mainGame->ebScale->setText(L"");
				switch(mainGame->cbCardType->getItemData(mainGame->cbCardType->getSelected())) {
				case CARD_TYPE_FILTER_ALL:
				case CARD_TYPE_FILTER_SKILL: {
					mainGame->cbRace->setEnabled(false);
					mainGame->cbAttribute->setEnabled(false);
					mainGame->ebAttack->setEnabled(false);
					mainGame->ebDefense->setEnabled(false);
					mainGame->ebStar->setEnabled(false);
					mainGame->ebScale->setEnabled(false);
					break;
				}
				case CARD_TYPE_FILTER_MONSTER: {
					mainGame->cbRace->setEnabled(true);
					mainGame->cbAttribute->setEnabled(true);
					mainGame->ebAttack->setEnabled(true);
					mainGame->ebDefense->setEnabled(true);
					mainGame->ebStar->setEnabled(true);
					mainGame->ebScale->setEnabled(true);
					break;
				}
				case CARD_TYPE_FILTER_SPELL: {
					mainGame->cbRace->setEnabled(false);
					mainGame->cbAttribute->setEnabled(false);
					mainGame->ebAttack->setEnabled(false);
					mainGame->ebDefense->setEnabled(false);
					mainGame->ebStar->setEnabled(false);
					mainGame->ebScale->setEnabled(false);
					break;
				}
				case CARD_TYPE_FILTER_TRAP: {
					mainGame->cbRace->setEnabled(false);
					mainGame->cbAttribute->setEnabled(false);
					mainGame->ebAttack->setEnabled(false);
					mainGame->ebDefense->setEnabled(false);
					mainGame->ebStar->setEnabled(false);
					mainGame->ebScale->setEnabled(false);
					break;
				}
				}
				StartFilter(true);
				break;
			}
			case COMBOBOX_SECONDTYPE:
			case COMBOBOX_OTHER_FILT: {
				if (id==COMBOBOX_SECONDTYPE && mainGame->cbCardType->getItemData(mainGame->cbCardType->getSelected()) == CARD_TYPE_FILTER_MONSTER) {
					if (mainGame->cbCardType2->getSelected() == 8) {
						mainGame->ebDefense->setEnabled(false);
						mainGame->ebDefense->setText(L"");
					} else {
						mainGame->ebDefense->setEnabled(true);
					}
				}
				StartFilter(true);
				break;
			}
			case COMBOBOX_SORTTYPE: {
				SortList();
				mainGame->env->setFocus(0);
				break;
			}
			}
			break;
		}
		case irr::gui::EGET_CHECKBOX_CHANGED: {
			switch (id) {
				case CHECKBOX_SHOW_ANIME: {
					int prevLimit = mainGame->cbLimit->getSelected();
					mainGame->ReloadCBLimit();
					if (prevLimit < 8)
						mainGame->cbLimit->setSelected(prevLimit);
					StartFilter(true);
					break;
				}
			}
			break;
		}
		default: break;
		}
		break;
	}
	case irr::EET_MOUSE_INPUT_EVENT: {
		bool isroot = mainGame->env->getRootGUIElement()->getElementFromPoint(mouse_pos) == mainGame->env->getRootGUIElement();
		const bool forceInput = gGameConfig->ignoreDeckContents || event.MouseInput.Shift;
		switch(event.MouseInput.Event) {
		case irr::EMIE_LMOUSE_PRESSED_DOWN: {
			if(is_draging || drag_pending)
				break;
			mouse_pos.set(event.MouseInput.X, event.MouseInput.Y);
			if(mainGame->env->getRootGUIElement()->getElementFromPoint(mouse_pos) != mainGame->env->getRootGUIElement())
				break;
			if(mainGame->wCategories->isVisible() || mainGame->wQuery->isVisible())
				break;
			GetHoveredCard();
			const auto logicalPress = mainGame->Resize(event.MouseInput.X, event.MouseInput.Y, true);
			if(results.size() > 6 && logicalPress.X >= 870 && logicalPress.X < 955 && logicalPress.Y >= 164 && logicalPress.Y < 634) {
				search_result_scroll_pending = true;
				search_result_scroll_active = false;
				search_result_scroll_start_x = logicalPress.X;
				search_result_scroll_start_y = logicalPress.Y;
				search_result_scroll_start_offset = search_result_scroll_offset;
				search_result_scroll_velocity = 0.0f;
				search_result_scroll_inertia = false;
				search_result_scroll_last_y = logicalPress.Y;
				search_result_scroll_last_time = mainGame->device->getTimer()->getTime();
				break;
			}
			const auto grid = mainGame->GetDeckGridLayout();
			const int gridBottom = grid.firstRowY + 2 * grid.rowStep + grid.cardHeight;
			const int panelLeft = mainGame->GetDeckPanelLeft();
			const int leftTrackRight = grid.gridLeft + (grid.slotWidth - grid.cardWidth) / 2;
			const int rightTrackLeft = grid.gridLeft + 4 * grid.slotWidth +
				(grid.slotWidth - grid.cardWidth) / 2 + grid.cardWidth;
			const bool inSideTrack = (logicalPress.X >= panelLeft && logicalPress.X < leftTrackRight) ||
				(logicalPress.X >= rightTrackLeft && logicalPress.X < 800);
			if(active_deck_tab == 0 && current_deck.main.size() >= 15 &&
			   logicalPress.Y >= grid.firstRowY && logicalPress.Y < gridBottom && inSideTrack) {
				deck_scroll_pending = true;
				deck_scroll_active = false;
				deck_scroll_free = true;
				deck_scroll_start_x = logicalPress.X;
				deck_scroll_start_y = logicalPress.Y;
				deck_scroll_start_offset = main_deck_scroll_offset;
				main_deck_scroll_velocity = 0.0f;
				main_deck_scroll_inertia = false;
				deck_scroll_last_y = logicalPress.Y;
				deck_scroll_last_time = mainGame->device->getTimer()->getTime();
			}
			if(hovered_pos == 0 || hovered_seq == -1)
				break;
			if(hovered_pos == 4) {
				search_result_scroll_velocity = 0.0f;
				search_result_scroll_inertia = false;
				const int firstRow = static_cast<int>(std::floor(search_result_scroll_offset / kDeckSearchResultRowHeight));
				const float rowOffset = search_result_scroll_offset - firstRow * kDeckSearchResultRowHeight;
				const int row = static_cast<int>(std::floor((logicalPress.Y - 165 + rowOffset) / kDeckSearchResultRowHeight));
				const float imageTop = 165.0f + row * kDeckSearchResultRowHeight - rowOffset;
				const bool onCardImage = logicalPress.X >= 810 && logicalPress.X < 810 + kDeckSearchResultThumbWidth &&
					logicalPress.Y >= imageTop && logicalPress.Y < imageTop + kDeckSearchResultThumbHeight;
				if(!onCardImage)
					break;
			}
			click_pos = hovered_pos;
			drag_start_seq = hovered_seq;
			drag_source_in_deck = click_pos >= 1 && click_pos <= 3;
			dragx = event.MouseInput.X;
			dragy = event.MouseInput.Y;
			if(!hovered_code || !(dragging_pointer = gDataManager->GetCardData(hovered_code)))
				break;
			if(hovered_pos == 4 && !forceInput && !check_limit(dragging_pointer))
				break;
			// A touch-down is only a candidate drag. Keep the card in its deck
			// until the pointer actually moves beyond the drag threshold.
			drag_pending = true;
			break;
		}
		case irr::EMIE_LMOUSE_LEFT_UP: {
			mouse_pos.set(event.MouseInput.X, event.MouseInput.Y);
			const bool releaseIsRoot = mainGame->env->getRootGUIElement()->getElementFromPoint(mouse_pos) == mainGame->env->getRootGUIElement();
			if(!releaseIsRoot) {
				search_result_scroll_pending = false;
				search_result_scroll_active = false;
				deck_scroll_pending = false;
				deck_scroll_active = false;
				deck_scroll_free = false;
				is_draging = false;
				drag_pending = false;
				drag_source_in_deck = false;
				drag_start_seq = -1;
				dragging_pointer = nullptr;
				break;
			}
			if(search_result_scroll_pending) {
				const auto logicalRelease = mainGame->Resize(event.MouseInput.X, event.MouseInput.Y, true);
				const int dx = logicalRelease.X - search_result_scroll_start_x;
				const int dy = logicalRelease.Y - search_result_scroll_start_y;
				if(!search_result_scroll_active && std::abs(dy) >= 10 && std::abs(dy) > std::abs(dx)) {
					search_result_scroll_active = true;
					is_draging = false;
					drag_pending = false;
					drag_source_in_deck = false;
					drag_start_seq = -1;
					dragging_pointer = nullptr;
				}
				if(search_result_scroll_active) {
					updateSearchResultFreeScroll(logicalRelease.Y);
					const uint32_t now = mainGame->device->getTimer()->getTime();
					const uint32_t elapsed = now - search_result_scroll_last_time;
					if(elapsed > 0 && elapsed <= 120 && logicalRelease.Y != search_result_scroll_last_y) {
						const float delta = static_cast<float>(logicalRelease.Y - search_result_scroll_last_y);
						search_result_scroll_velocity = std::clamp(-delta * 1000.0f / static_cast<float>(elapsed), -1800.0f, 1800.0f);
					}
					if(std::abs(search_result_scroll_velocity) > 45.0f)
						search_result_scroll_inertia = true;
				}
				search_result_scroll_pending = false;
				search_result_scroll_active = false;
				drag_pending = false;
				drag_start_seq = -1;
				dragging_pointer = nullptr;
				GetHoveredCard();
				break;
			}
			if(deck_scroll_pending) {
				const auto logicalRelease = mainGame->Resize(event.MouseInput.X, event.MouseInput.Y, true);
				const int dx = logicalRelease.X - deck_scroll_start_x;
				const int dy = logicalRelease.Y - deck_scroll_start_y;
				if(std::abs(dy) >= 10 && std::abs(dy) > std::abs(dx)) {
					deck_scroll_active = true;
					is_draging = false;
					drag_pending = false;
					drag_source_in_deck = false;
					drag_start_seq = -1;
					dragging_pointer = nullptr;
					updateMainDeckFreeScroll(logicalRelease.Y);
				}
			}
			if(deck_scroll_active) {
				if(deck_scroll_free && std::abs(main_deck_scroll_velocity) > 0.12f)
					main_deck_scroll_inertia = true;
				deck_scroll_pending = false;
				deck_scroll_active = false;
				deck_scroll_free = false;
				drag_pending = false;
				drag_start_seq = -1;
				dragging_pointer = nullptr;
				GetHoveredCard();
				break;
			}
			if(deck_scroll_pending) {
				deck_scroll_pending = false;
				if(deck_scroll_free) {
					deck_scroll_free = false;
					drag_pending = false;
					drag_start_seq = -1;
					dragging_pointer = nullptr;
					GetHoveredCard();
					break;
				}
				deck_scroll_free = false;
			}
			// Some touch drivers report the final position only on release.
			if(drag_pending) {
				const int threshold = std::max(8, mainGame->Scale(10));
				const int dx = event.MouseInput.X - dragx;
				const int dy = event.MouseInput.Y - dragy;
				if(dx * dx + dy * dy >= threshold * threshold) {
					drag_pending = false;
					is_draging = true;
				}
			}
			if(!is_draging) {
				drag_pending = false;
				drag_start_seq = -1;
				dragging_pointer = nullptr;
				GetHoveredCard();
				break;
			}
			// The fullscreen artwork is a preview surface, not a deck drop target.
			// Cancel the drag here so an underlying deck slot cannot receive it.
			if(mainGame->cardViewFullscreen && mainGame->wCardImg->isVisible() &&
			   mainGame->imgCard->isVisible() &&
			   mainGame->imgCard->getAbsolutePosition().isPointInside(mouse_pos)) {
				is_draging = false;
				drag_pending = false;
				drag_source_in_deck = false;
				drag_start_seq = -1;
				dragging_pointer = nullptr;
				GetHoveredCard();
				break;
			}
			GetHoveredCard();
			const int targetPos = hovered_pos;
			// Dropping a card from a deck onto the Search Result area removes it
			// from its source deck, matching the editor's original removal gesture.
			if(targetPos == 4 && drag_source_in_deck) {
				if(click_pos == 1)
					pop_main(drag_start_seq);
				else if(click_pos == 2)
					pop_extra(drag_start_seq);
				else if(click_pos == 3)
					pop_side(drag_start_seq);
				is_draging = false;
				drag_pending = false;
				drag_source_in_deck = false;
				drag_start_seq = -1;
				dragging_pointer = nullptr;
				GetHoveredCard();
				break;
			}
			// Other invalid targets cancel the drag without changing the deck.
			if(targetPos < 1 || targetPos > 3) {
				is_draging = false;
				drag_pending = false;
				drag_source_in_deck = false;
				drag_start_seq = -1;
				dragging_pointer = nullptr;
				break;
			}
			bool pushed = false;
			int targetSeq = hovered_seq;
			int targetLast = is_lastcard;
			if(drag_source_in_deck) {
				if(click_pos == 1)
					pop_main(drag_start_seq);
				else if(click_pos == 2)
					pop_extra(drag_start_seq);
				else if(click_pos == 3)
					pop_side(drag_start_seq);
				if(targetPos == click_pos) {
					if(targetSeq > drag_start_seq)
						--targetSeq;
					else if(targetSeq == drag_start_seq)
						targetLast = 0;
				}
			}
			if(targetPos == 1)
				pushed = push_main(dragging_pointer, targetSeq + targetLast, forceInput);
			else if(targetPos == 2)
				pushed = push_extra(dragging_pointer, targetSeq + targetLast, forceInput);
			else if(targetPos == 3)
				pushed = push_side(dragging_pointer, targetSeq + targetLast, forceInput);
			if(!pushed && drag_source_in_deck) {
				if(click_pos == 1)
					push_main(dragging_pointer, drag_start_seq, true);
				else if(click_pos == 2)
					push_extra(dragging_pointer, drag_start_seq, true);
				else if(click_pos == 3)
					push_side(dragging_pointer, drag_start_seq, true);
				pushed = true;
			}

			is_draging = false;
			drag_pending = false;
			drag_source_in_deck = false;
			drag_start_seq = -1;
			dragging_pointer = nullptr;
			break;
		}
		case irr::EMIE_RMOUSE_LEFT_UP: {
			if(!isroot)
				break;
			if(mainGame->is_siding) {
				if(is_draging)
					break;
				if(hovered_pos == 0 || hovered_seq == -1)
					break;
				auto pointer = gDataManager->GetCardData(hovered_code);
				if(!pointer)
					break;
				if(hovered_pos == 1) {
					if(push_side(pointer))
						pop_main(hovered_seq);
				} else if(hovered_pos == 2) {
					if(push_side(pointer))
						pop_extra(hovered_seq);
				} else {
					if(push_extra(pointer) || push_main(pointer))
						pop_side(hovered_seq);
				}
				break;
			}
			if(mainGame->wCategories->isVisible() || mainGame->wQuery->isVisible())
				break;
			if(!is_draging) {
				if(hovered_pos == 0 || hovered_seq == -1)
					break;
				if(hovered_pos == 1) {
					pop_main(hovered_seq);
				} else if(hovered_pos == 2) {
					pop_extra(hovered_seq);
				} else if(hovered_pos == 3) {
					pop_side(hovered_seq);
				} else {
					auto pointer = gDataManager->GetCardData(hovered_code);
					if(!pointer || (!gGameConfig->ignoreDeckContents && !check_limit(pointer)))
						break;
					if (event.MouseInput.Shift) {
						push_side(pointer, -1, gGameConfig->ignoreDeckContents);
					}
					else {
						if (!push_main(pointer, -1, gGameConfig->ignoreDeckContents) && !push_extra(pointer, -1, gGameConfig->ignoreDeckContents))
							push_side(pointer);
					}
				}
			} else {
				if(drag_source_in_deck) {
					if(click_pos == 1) pop_main(drag_start_seq);
					else if(click_pos == 2) pop_extra(drag_start_seq);
					else if(click_pos == 3) pop_side(drag_start_seq);
				}
				bool moved = false;
				if(click_pos == 1 || click_pos == 2) moved = push_side(dragging_pointer);
				else if(click_pos == 3) moved = push_extra(dragging_pointer) || push_main(dragging_pointer);
				else moved = push_side(dragging_pointer);
				if(!moved && drag_source_in_deck) {
					if(click_pos == 1) push_main(dragging_pointer, drag_start_seq, true);
					else if(click_pos == 2) push_extra(dragging_pointer, drag_start_seq, true);
					else if(click_pos == 3) push_side(dragging_pointer, drag_start_seq, true);
				}
				is_draging = false;
				drag_source_in_deck = false;
				drag_pending = false;
				drag_start_seq = -1;
				dragging_pointer = nullptr;
			}
			break;
		}
		case irr::EMIE_MMOUSE_LEFT_UP: {
			if(!isroot)
				break;
			if (mainGame->is_siding)
				break;
			if (mainGame->wCategories->isVisible() || mainGame->wQuery->isVisible())
				break;
			if (hovered_pos == 0 || hovered_seq == -1)
				break;
			if (is_draging)
				break;
			auto pointer = gDataManager->GetCardData(hovered_code);
			if(!pointer || (!forceInput && !check_limit(pointer)))
				break;
			if (hovered_pos == 1) {
				if(!push_main(pointer))
					push_side(pointer);
			} else if (hovered_pos == 2) {
				if(!push_extra(pointer))
					push_side(pointer);
			} else if (hovered_pos == 3) {
				if(!push_side(pointer) && !push_extra(pointer))
					push_main(pointer);
			} else {
				if(!push_extra(pointer) && !push_main(pointer))
					push_side(pointer);
			}
			break;
		}
		case irr::EMIE_MOUSE_MOVED: {
			mouse_pos.set(event.MouseInput.X, event.MouseInput.Y);
			if(search_result_scroll_pending) {
				const auto logicalMove = mainGame->Resize(event.MouseInput.X, event.MouseInput.Y, true);
				const int dx = logicalMove.X - search_result_scroll_start_x;
				const int dy = logicalMove.Y - search_result_scroll_start_y;
				if(!search_result_scroll_active && std::abs(dy) >= 10 && std::abs(dy) > std::abs(dx)) {
					search_result_scroll_active = true;
					is_draging = false;
					drag_pending = false;
					drag_source_in_deck = false;
					drag_start_seq = -1;
					dragging_pointer = nullptr;
				}
				if(search_result_scroll_active) {
					updateSearchResultFreeScroll(logicalMove.Y);
					const uint32_t now = mainGame->device->getTimer()->getTime();
					const uint32_t elapsed = now - search_result_scroll_last_time;
					if(elapsed > 0 && elapsed <= 120) {
						const float delta = static_cast<float>(logicalMove.Y - search_result_scroll_last_y);
						search_result_scroll_velocity = std::clamp(-delta * 1000.0f / static_cast<float>(elapsed), -1800.0f, 1800.0f);
					}
					search_result_scroll_last_y = logicalMove.Y;
					search_result_scroll_last_time = now;
					GetHoveredCard();
				}
				break;
			}
			if(deck_scroll_pending) {
				const auto logicalMove = mainGame->Resize(event.MouseInput.X, event.MouseInput.Y, true);
				const int dx = logicalMove.X - deck_scroll_start_x;
				const int dy = logicalMove.Y - deck_scroll_start_y;
				if(std::abs(dy) >= 10 && std::abs(dy) > std::abs(dx)) {
					deck_scroll_active = true;
					is_draging = false;
					drag_pending = false;
					drag_source_in_deck = false;
					drag_start_seq = -1;
					dragging_pointer = nullptr;
					updateMainDeckFreeScroll(logicalMove.Y);
				} else if(std::abs(dx) >= 10 && std::abs(dx) > std::abs(dy)) {
					deck_scroll_pending = false;
					deck_scroll_free = false;
				}
			}
			if(deck_scroll_active) {
				const auto logicalMove = mainGame->Resize(event.MouseInput.X, event.MouseInput.Y, true);
				if(deck_scroll_free) {
					updateMainDeckFreeScroll(logicalMove.Y);
					const uint32_t now = mainGame->device->getTimer()->getTime();
					const uint32_t elapsed = now - deck_scroll_last_time;
					if(elapsed > 0 && elapsed <= 120) {
						const float deltaRows = static_cast<float>(logicalMove.Y - deck_scroll_last_y) /
							static_cast<float>(std::max(1, mainGame->GetDeckGridLayout().rowStep));
						main_deck_scroll_velocity = std::clamp(
							-deltaRows * 1000.0f / static_cast<float>(elapsed), -8.0f, 8.0f);
					}
					deck_scroll_last_y = logicalMove.Y;
					deck_scroll_last_time = now;
				}
				GetHoveredCard();
				break;
			}
			if(drag_pending) {
				const int threshold = std::max(8, mainGame->Scale(10));
				const int dx = event.MouseInput.X - dragx;
				const int dy = event.MouseInput.Y - dragy;
				if(dx * dx + dy * dy >= threshold * threshold) {
					drag_pending = false;
					is_draging = true;
				}
			}
			GetHoveredCard();
			break;
		}
		case irr::EMIE_MOUSE_WHEEL: {
			if(!mainGame->Resize(805, 160, 1024 - kUiEdgeMargin, 640 - kUiEdgeMargin).isPointInside(mouse_pos))
				break;
			search_result_scroll_inertia = false;
			search_result_scroll_velocity = 0.0f;
			search_result_scroll_offset = std::clamp(search_result_scroll_offset + (event.MouseInput.Wheel < 0 ?
				static_cast<float>(kDeckSearchResultRowHeight) : -static_cast<float>(kDeckSearchResultRowHeight)), 0.0f, getSearchResultMaxScroll());
			GetHoveredCard();
			return true;
		}
		default: break;
		}
		break;
	}
	case irr::EET_KEY_INPUT_EVENT: {
		if(event.KeyInput.PressedDown && event.KeyInput.Key == irr::KEY_ESCAPE && deck_name_input_mode != 0) {
			CancelDeckNameInput();
			return true;
		}
		if(event.KeyInput.PressedDown && !mainGame->HasFocus(irr::gui::EGUIET_EDIT_BOX)) {
			switch(event.KeyInput.Key) {
			case irr::KEY_KEY_C: {
				if(event.KeyInput.Control)
					ExportDeckToClipboard(event.KeyInput.Shift);
				break;
			}
			case irr::KEY_KEY_V: {
				if(event.KeyInput.Control && !mainGame->HasFocus(irr::gui::EGUIET_EDIT_BOX))
					ImportDeck();
				break;
			}
			default:
				break;
			}
		}
		break;
	}
#if !EDOPRO_ANDROID && !EDOPRO_IOS
	case irr::EET_DROP_EVENT: {
		static std::wstring to_open_file;
		switch(event.DropEvent.DropType) {
			case irr::DROP_FILE: {
				irr::gui::IGUIElement* root = mainGame->env->getRootGUIElement();
				if(root->getElementFromPoint({ event.DropEvent.X, event.DropEvent.Y }) != root)
					break;
				to_open_file = event.DropEvent.Text;
				break;
			}
			case irr::DROP_TEXT: {
				if(!event.DropEvent.Text)
					break;
				if(mainGame->is_siding)
					break;
				irr::gui::IGUIElement* root = mainGame->env->getRootGUIElement();
				if(root->getElementFromPoint({ event.DropEvent.X, event.DropEvent.Y }) != root)
					break;
				epro::wstringview text{ event.DropEvent.Text };
				if(starts_with(text, L"ydke://")) {
					gdeckManager->ImportDeckYdke(current_deck, text);
					return true;
				}
				if(gdeckManager->ImportDeckBase64Omega(current_deck, text))
					return true;
				std::wstringstream ss(Utils::ToUpperNoAccents(text));
				std::wstring to;
				int firstcode = 0;
				while(std::getline(ss, to)) {
					auto pos = to.find_first_not_of(L" \n\r\t");
					if(pos != std::wstring::npos && pos != 0)
						to.erase(to.begin(), to.begin() + pos);
					pos = to.find_last_not_of(L" \n\r\t");
					if(pos != std::wstring::npos) {
						if(pos < to.size())
							pos++;
						to.erase(pos);
					}
					auto* chbuff = to.data();
					uint32_t code = BufferIO::GetVal(*chbuff == L'C' ? chbuff + 1 : chbuff);
					const CardDataC* pointer = nullptr;
					if(!code || !(pointer = gDataManager->GetCardData(code))) {
						for(auto& card : gDataManager->cards) {
							const auto& name = card.second.GetStrings().uppercase_name;
							if(name == to) {
								pointer = &card.second._data;
								break;
							}
						}
					}
					if(!pointer)
						continue;
					if(!firstcode)
						firstcode = pointer->code;
					mouse_pos.set(event.DropEvent.X, event.DropEvent.Y);
					is_draging = true;
					dragging_pointer = pointer;
					GetHoveredCard();
					if(hovered_pos == 3)
						push_side(dragging_pointer, hovered_seq + is_lastcard, true);
					else {
						push_main(dragging_pointer, hovered_seq + is_lastcard, true) || push_extra(dragging_pointer, hovered_seq + is_lastcard, true);
					}
					is_draging = false;
				}
				if(firstcode)
					mainGame->ShowCardInfo(firstcode);
				return true;
			}
			case irr::DROP_END: {
				if(to_open_file.size()) {
					auto extension = Utils::GetFileExtension(to_open_file);
					if(!mainGame->is_siding && extension == L"ydk" && mainGame->deckBuilder.SetCurrentDeckFromFile(Utils::ToPathString(to_open_file), true)) {
						auto name = Utils::GetFileName(to_open_file);
						mainGame->ebDeckname->setText(name.data());
						mainGame->cbDBDecks->setSelected(-1);
					} else if(extension == L"pem" || extension == L"cer" || extension == L"crt") {
						gGameConfig->override_ssl_certificate_path = BufferIO::EncodeUTF8(to_open_file);
					}
					to_open_file.clear();
				}
				break;
			}
			default: break;
		}
		break;
	}
#endif
	default: break;
	}
	return false;
}
void DeckBuilder::UpdateMainDeckScroll(uint32_t deltaTime) {
	if(!main_deck_scroll_inertia || mainGame->is_siding || !mainGame->is_building)
		return;
	const float dt = std::min(0.05f, static_cast<float>(deltaTime) / 1000.0f);
	if(dt <= 0.0f)
		return;
	const int unlockedRows = std::max(3, static_cast<int>(current_deck.main.size() / 5) + 1);
	const float maxScroll = static_cast<float>(unlockedRows - 3);
	main_deck_scroll_offset = std::clamp(main_deck_scroll_offset + main_deck_scroll_velocity * dt, 0.0f, maxScroll);
	if(main_deck_scroll_offset <= 0.0f || main_deck_scroll_offset >= maxScroll)
		main_deck_scroll_velocity = 0.0f;
	main_deck_scroll_velocity *= std::exp(-5.5f * dt);
	if(std::abs(main_deck_scroll_velocity) < 0.035f) {
		main_deck_scroll_velocity = 0.0f;
		main_deck_scroll_inertia = false;
	}
	GetHoveredCard();
}

void DeckBuilder::UpdateSearchResultScroll(uint32_t deltaTime) {
	if(!search_result_scroll_inertia || mainGame->is_siding || !mainGame->is_building)
		return;
	const float dt = std::min(0.05f, static_cast<float>(deltaTime) / 1000.0f);
	if(dt <= 0.0f)
		return;
	const float maxScroll = std::max(0.0f, static_cast<float>(results.size()) * kDeckSearchResultRowHeight - kDeckSearchResultViewportHeight);
	search_result_scroll_offset = std::clamp(search_result_scroll_offset + search_result_scroll_velocity * dt, 0.0f, maxScroll);
	if(search_result_scroll_offset <= 0.0f || search_result_scroll_offset >= maxScroll)
		search_result_scroll_velocity = 0.0f;
	search_result_scroll_velocity *= std::exp(-5.5f * dt);
	if(std::abs(search_result_scroll_velocity) < 8.0f) {
		search_result_scroll_velocity = 0.0f;
		search_result_scroll_inertia = false;
	}
	GetHoveredCard();
}

void DeckBuilder::GetHoveredCard() {
	irr::gui::IGUIElement* root = mainGame->env->getRootGUIElement();
	if(root->getElementFromPoint(mouse_pos) != root)
		return;
	auto relative_mouse_pos = mainGame->Resize(mouse_pos.X, mouse_pos.Y, true);
	auto x = relative_mouse_pos.X;
	auto y = relative_mouse_pos.Y;
	const irr::core::recti searchResultRect{ 810, 165, 995, 631 };
	auto pre_code = hovered_code;
	hovered_seq = -1;
	hovered_pos = 0;
	hovered_code = 0;
	is_lastcard = 0;

	auto UpdateHoverCode = [&]() {
		if(searchResultRect.isPointInside(relative_mouse_pos)) {
			hovered_pos = 4;
			if(results.empty())
				return;
			const int firstRow = static_cast<int>(std::floor(search_result_scroll_offset / kDeckSearchResultRowHeight));
			const float rowOffset = search_result_scroll_offset - firstRow * kDeckSearchResultRowHeight;
			const int seq = static_cast<int>(std::floor((y - 165 + rowOffset) / kDeckSearchResultRowHeight));
			const int pos = firstRow + seq;

			if(pos < 0 || pos >= static_cast<int>(results.size()))
				return;

			hovered_seq = seq;
			hovered_code = results[pos]->code;
			return;
		}

		const auto grid = mainGame->GetDeckGridLayout();
		float scrollRows = 0.0f;
		if(active_deck_tab == 0) {
			const int unlockedRows = std::max(3, static_cast<int>(current_deck.main.size() / 5) + 1);
			main_deck_scroll_offset = std::clamp(main_deck_scroll_offset,
				0.0f, static_cast<float>(unlockedRows - 3));
			scrollRows = main_deck_scroll_offset;
		}
		const int panelLeft = grid.gridLeft;
		const int panelRight = panelLeft + grid.slotWidth * 5;
		if(x < panelLeft || x >= panelRight || y < grid.firstRowY)
			return;

		const int row = static_cast<int>(std::floor(
			static_cast<float>(y - grid.firstRowY) / std::max(1, grid.rowStep) + scrollRows));
		const int screenCardY = grid.firstRowY + static_cast<int>(std::round(
			(static_cast<float>(row) - scrollRows) * grid.rowStep));
		const int availableRows = active_deck_tab == 0
			? std::max(3, static_cast<int>(current_deck.main.size() / 5) + 1)
			: 3;
		if(row < 0 || row >= availableRows || y < screenCardY || y >= screenCardY + grid.cardHeight ||
		   y >= grid.firstRowY + 2 * grid.rowStep + grid.cardHeight)
			return;

		hovered_pos = active_deck_tab + 1;
		const Deck::Vector* activeDeck = active_deck_tab == 0 ? &current_deck.main :
			(active_deck_tab == 1 ? &current_deck.extra : &current_deck.side);
		const int col = (x - panelLeft) / grid.slotWidth;
		const int seq = row * 5 + col;
		if(seq < static_cast<int>(activeDeck->size())) {
			hovered_seq = seq;
			hovered_code = (*activeDeck)[seq]->code;
			const int cardLeft = grid.gridLeft + col * grid.slotWidth + (grid.slotWidth - grid.cardWidth) / 2;
			is_lastcard = x >= cardLeft + grid.cardWidth / 2;
		} else {
			hovered_seq = activeDeck->empty() ? -1 : static_cast<int>(activeDeck->size()) - 1;
			is_lastcard = 1;
		}
	};
	UpdateHoverCode();
	if(is_draging) {
		dragx = mouse_pos.X;
		dragy = mouse_pos.Y;
	}
	if(!is_draging && pre_code != hovered_code) {
		if(hovered_code)
			mainGame->ShowCardInfo(hovered_code);
	}
}
#define CHECK_AND_SET(x) if(x != prev_##x) {\
	res = true;\
	}\
	prev_##x = x;
bool DeckBuilder::FiltersChanged() {
	bool res = false;
	CHECK_AND_SET(filter_effect);
	CHECK_AND_SET(filter_type);
	CHECK_AND_SET(filter_type2);
	CHECK_AND_SET(filter_attrib);
	CHECK_AND_SET(filter_race);
	CHECK_AND_SET(filter_atktype);
	CHECK_AND_SET(filter_atk);
	CHECK_AND_SET(filter_deftype);
	CHECK_AND_SET(filter_def);
	CHECK_AND_SET(filter_lvtype);
	CHECK_AND_SET(filter_lv);
	CHECK_AND_SET(filter_scltype);
	CHECK_AND_SET(filter_scl);
	CHECK_AND_SET(filter_marks);
	CHECK_AND_SET(filter_lm);
	return res;
}
#undef CHECK_AND_SET
void DeckBuilder::StartFilter(bool force_refresh) {
	filter_type = mainGame->cbCardType->getItemData(mainGame->cbCardType->getSelected());
	filter_type2 = mainGame->cbCardType2->getItemData(mainGame->cbCardType2->getSelected());
	filter_lm = static_cast<limitation_search_filters>(mainGame->cbLimit->getItemData(mainGame->cbLimit->getSelected()));
	if(filter_type == CARD_TYPE_FILTER_MONSTER) {
		filter_attrib = mainGame->cbAttribute->getItemData(mainGame->cbAttribute->getSelected());
		auto selected = mainGame->cbRace->getItemData(mainGame->cbRace->getSelected());
		if(selected == 0)
			filter_race = 0;
		else
			filter_race = UINT64_C(1) << (selected - 1);
		filter_atk = parse_filter(mainGame->ebAttack->getText(), filter_atktype);
		filter_def = parse_filter(mainGame->ebDefense->getText(), filter_deftype);
		filter_lv = parse_filter(mainGame->ebStar->getText(), filter_lvtype);
		filter_scl = parse_filter(mainGame->ebScale->getText(), filter_scltype);
	}
	FilterCards(force_refresh);
	GetHoveredCard();
}
void DeckBuilder::FilterCards(bool force_refresh) {
	results.clear();
	std::vector<epro::wstringview> searchterms;
	const auto uppercase_text = Utils::ToUpperNoAccents(mainGame->ebCardName->getText());
	if(!uppercase_text.empty()) {
		searchterms = Utils::TokenizeString<epro::wstringview>(uppercase_text, L"||");
	} else {
		searchterms = { L"" };
	}
	if(FiltersChanged() || force_refresh)
		searched_terms.clear();
	if(!searched_terms.empty()) {
		//removes search terms already cached
		for(auto it = searchterms.cbegin(); it != searchterms.cend();) {
			if(searched_terms.find((*it)) != searched_terms.end())
				it = searchterms.erase(it);
			else
				it++;
		}
	}
	//removes duplicate search terms
	for(auto it = searchterms.cbegin(); it != searchterms.cend();) {
		if(auto found_it = std::find(searchterms.cbegin(), searchterms.cend(), *it); found_it != it)
			it = searchterms.erase(it);
		else
			it++;
	}
	//removes no longer existing search terms from the cache
	if(!searchterms.empty()) {
		for(auto it = searched_terms.cbegin(); it != searched_terms.cend();) {
			if(std::find(searchterms.begin(), searchterms.end(), it->first) == searchterms.end())
				it = searched_terms.erase(it);
			else
				++it;
		}
	}
	for(const auto& term_ : searchterms) {
		int trycode = BufferIO::GetVal(term_.data());
		const CardDataC* data = nullptr;
		if(trycode && (data = gDataManager->GetCardData(trycode))) {
			auto it = searched_terms.find(term_);
			if(it != searched_terms.end()) {
				it->second = { data };
			} else {
				searched_terms.emplace(std::wstring{ term_ }, std::vector{ data });
			}
			continue;
		}
		auto subterms = Utils::TokenizeString<epro::wstringview>(term_, L"&&");
		std::vector<SearchParameter> search_parameters;
		search_parameters.reserve(subterms.size());
		bool would_return_nothing = false;
		for(auto subterm : subterms) {
			std::vector<epro::wstringview> tokens;
			int modif = 0;
			if(!subterm.empty()) {
				if(starts_with(subterm, L"!!")) {
					modif |= SEARCH_MODIFIER_NEGATIVE_LOOKUP;
					subterm.remove_prefix(2);
				}
				if(starts_with(subterm, L'@')) {
					modif |= SEARCH_MODIFIER_ARCHETYPE_ONLY;
					subterm.remove_prefix(1);
				} else if(starts_with(subterm, L"$$")) {
					modif |= SEARCH_MODIFIER_TEXT_ONLY;
					subterm.remove_prefix(1);
				} else if(starts_with(subterm, L'$')) {
					modif |= SEARCH_MODIFIER_NAME_ONLY;
					subterm.remove_prefix(1);
				}
				tokens = Utils::TokenizeString<epro::wstringview>(subterm, L'*');
			}
			if(tokens.empty()) {
				if((modif & (SEARCH_MODIFIER_NEGATIVE_LOOKUP | SEARCH_MODIFIER_ARCHETYPE_ONLY)) == 0)
					continue;
				// an empty token set, actually matters when filtering for archetype only
				// there do exist cards with no archetypes
				if((modif & SEARCH_MODIFIER_ARCHETYPE_ONLY) == 0) {
					would_return_nothing = true;
					break;
				}
			}
			std::vector<uint16_t> setcodes;
			if((modif & (SEARCH_MODIFIER_NAME_ONLY | SEARCH_MODIFIER_TEXT_ONLY)) == 0) {
				setcodes = gDataManager->GetSetCode(tokens);
				// no valid setcode found, either it will return everything (if negative lookup is used), or it will return nothing
				if(tokens.size() && setcodes.empty() && (modif & SEARCH_MODIFIER_ARCHETYPE_ONLY)) {
					would_return_nothing = ((modif & SEARCH_MODIFIER_NEGATIVE_LOOKUP) == 0);
					break;
				}
			}
			search_parameters.push_back(SearchParameter{std::move(tokens), std::move(setcodes), static_cast<SEARCH_MODIFIER>(modif)});
		}
		if(would_return_nothing)
			continue;
		std::vector<const CardDataC*> searchterm_results;
		for(const auto& card : gDataManager->cards) {
			if(!CheckCardProperties(card.second))
				continue;
			for(const auto& search_parameter : search_parameters) {
				if(!CheckCardText(card.second, search_parameter))
					goto skip;
			}
			searchterm_results.push_back(&card.second._data);
		skip:;
		}
		if(searchterm_results.size()) {
			auto it = searched_terms.find(term_);
			if(it != searched_terms.end()) {
				it->second.swap(searchterm_results);
			} else {
				searched_terms.emplace(std::wstring{ term_ }, std::move(searchterm_results));
			}
		}
	}
	for(const auto& [term, individual_results] : searched_terms) {
		results.reserve(results.size() + individual_results.size());
		results.insert(results.end(), individual_results.begin(), individual_results.end());
	}
	SortList();
	auto ip = std::unique(results.begin(), results.end());
	results.resize(std::distance(results.begin(), ip));
	result_string = epro::to_wstring(results.size());
	scroll_pos = 0;
	search_result_scroll_offset = 0.0f;
	search_result_scroll_velocity = 0.0f;
	search_result_scroll_inertia = false;
	search_result_scroll_pending = false;
	search_result_scroll_active = false;
}
bool DeckBuilder::CheckCardProperties(const CardDataM& data) {
	if(data._data.type & TYPE_TOKEN || data._data.ot & SCOPE_HIDDEN || ((data._data.ot & SCOPE_OFFICIAL) != data._data.ot && (!mainGame->chkAnime->isChecked() && !filterList->whitelist)))
		return false;
	switch(filter_type) {
	case CARD_TYPE_FILTER_MONSTER: {
		if(!(data._data.type & TYPE_MONSTER) || (data._data.type & filter_type2) != filter_type2)
			return false;
		if(filter_race && data._data.race != filter_race)
			return false;
		if(filter_attrib && data._data.attribute != filter_attrib)
			return false;
		if(filter_atktype) {
			if((filter_atktype == 1 && data._data.attack != filter_atk) || (filter_atktype == 2 && data._data.attack < filter_atk)
				|| (filter_atktype == 3 && data._data.attack <= filter_atk) || (filter_atktype == 4 && (data._data.attack > filter_atk || data._data.attack < 0))
				|| (filter_atktype == 5 && (data._data.attack >= filter_atk || data._data.attack < 0)) || (filter_atktype == 6 && data._data.attack != -2))
				return false;
		}
		if(filter_deftype) {
			if((filter_deftype == 1 && data._data.defense != filter_def) || (filter_deftype == 2 && data._data.defense < filter_def)
				|| (filter_deftype == 3 && data._data.defense <= filter_def) || (filter_deftype == 4 && (data._data.defense > filter_def || data._data.defense < 0))
				|| (filter_deftype == 5 && (data._data.defense >= filter_def || data._data.defense < 0)) || (filter_deftype == 6 && data._data.defense != -2)
				|| (data._data.type & TYPE_LINK))
				return false;
		}
		if(filter_lvtype) {
			if((filter_lvtype == 1 && data._data.level != filter_lv) || (filter_lvtype == 2 && data._data.level < filter_lv)
				|| (filter_lvtype == 3 && data._data.level <= filter_lv) || (filter_lvtype == 4 && data._data.level > filter_lv)
				|| (filter_lvtype == 5 && data._data.level >= filter_lv) || filter_lvtype == 6)
				return false;
		}
		if(filter_scltype) {
			if((filter_scltype == 1 && data._data.lscale != filter_scl) || (filter_scltype == 2 && data._data.lscale < filter_scl)
				|| (filter_scltype == 3 && data._data.lscale <= filter_scl) || (filter_scltype == 4 && (data._data.lscale > filter_scl))
				|| (filter_scltype == 5 && (data._data.lscale >= filter_scl)) || filter_scltype == 6
				|| !(data._data.type & TYPE_PENDULUM))
				return false;
		}
		break;
	}
	case CARD_TYPE_FILTER_SPELL: {
		if(!(data._data.type & TYPE_SPELL))
			return false;
		if(filter_type2 && data._data.type != filter_type2)
			return false;
		break;
	}
	case CARD_TYPE_FILTER_TRAP: {
		if(!(data._data.type & TYPE_TRAP))
			return false;
		if(filter_type2 && data._data.type != filter_type2)
			return false;
		break;
	}
	case CARD_TYPE_FILTER_SKILL: {
		if(!(data._data.type & TYPE_SKILL))
			return false;
		break;
	}
	}
	if(filter_effect && !(data._data.category & filter_effect))
		return false;
	if(filter_marks && (data._data.link_marker & filter_marks) != filter_marks)
		return false;
	if((filter_lm != LIMITATION_FILTER_NONE || filterList->whitelist) && filter_lm != LIMITATION_FILTER_ALL) {
		auto flit = filterList->GetLimitationIterator(&data._data);
		int count = 3;
		if(flit == filterList->content.end()) {
			if(filterList->whitelist)
				count = -1;
		} else
			count = flit->second;
		switch(filter_lm) {
			case LIMITATION_FILTER_BANNED:
			case LIMITATION_FILTER_LIMITED:
			case LIMITATION_FILTER_SEMI_LIMITED:
				if(count != filter_lm - 1)
					return false;
				break;
			case LIMITATION_FILTER_UNLIMITED:
				if(count < 3)
					return false;
				break;
			case LIMITATION_FILTER_OCG:
				if(data._data.ot != SCOPE_OCG)
					return false;
				break;
			case LIMITATION_FILTER_TCG:
				if(data._data.ot != SCOPE_TCG)
					return false;
				break;
			case LIMITATION_FILTER_TCG_OCG:
				if(data._data.ot != SCOPE_OCG_TCG)
					return false;
				break;
			case LIMITATION_FILTER_PRERELEASE:
				if(!(data._data.ot & SCOPE_PRERELEASE))
					return false;
				break;
			case LIMITATION_FILTER_SPEED:
				if(!(data._data.ot & SCOPE_SPEED))
					return false;
				break;
			case LIMITATION_FILTER_RUSH:
				if(!(data._data.ot & SCOPE_RUSH))
					return false;
				break;
			case LIMITATION_FILTER_LEGEND:
				if(!(data._data.ot & SCOPE_LEGEND))
					return false;
				break;
			case LIMITATION_FILTER_ANIME:
				if(data._data.ot != SCOPE_ANIME)
					return false;
				break;
			case LIMITATION_FILTER_ILLEGAL:
				if(data._data.ot != SCOPE_ILLEGAL)
					return false;
				break;
			case LIMITATION_FILTER_VIDEOGAME:
				if(data._data.ot != SCOPE_VIDEO_GAME)
					return false;
				break;
			case LIMITATION_FILTER_CUSTOM:
				if(!(data._data.ot & SCOPE_CUSTOM))
					return false;
				break;
			default:
				break;
		}
		if(filterList->whitelist && count < 0)
			return false;
	}
	return true;
}
static const auto& CardSetcodes(const CardDataC& data) {
	if(data.alias) {
		if(auto _data = gDataManager->GetCardData(data.alias); _data)
			return _data->setcodes;
	}
	return data.setcodes;
}
static bool check_set_code(const std::vector<uint16_t>& card_setcodes, const std::vector<uint16_t>& setcodes) {
	if(setcodes.empty())
		return card_setcodes.empty();
	for(auto& set_code : setcodes) {
		if(std::find(card_setcodes.begin(), card_setcodes.end(), set_code) != card_setcodes.end())
			return true;
	}
	return false;
}
bool DeckBuilder::CheckCardText(const CardDataM& data, const SearchParameter& search_parameter) {
	const auto checkNeg = [negative = !!(search_parameter.modifier & SEARCH_MODIFIER_NEGATIVE_LOOKUP)](bool res) -> bool {
		if(negative)
			return !res;
		return res;
	};
	const auto& strings = data.GetStrings();
	if(search_parameter.modifier & SEARCH_MODIFIER_NAME_ONLY) {
		return checkNeg(Utils::ContainsSubstring(strings.uppercase_name, search_parameter.tokens));
	} else if(search_parameter.modifier & SEARCH_MODIFIER_TEXT_ONLY) {
		return checkNeg(Utils::ContainsSubstring(strings.uppercase_text, search_parameter.tokens));
	} else if(search_parameter.modifier & SEARCH_MODIFIER_ARCHETYPE_ONLY) {
		const auto& setcodes = CardSetcodes(data._data);
		if(search_parameter.setcodes.empty())
			return checkNeg(!setcodes.empty());
		return checkNeg(check_set_code(setcodes, search_parameter.setcodes));
	} else {
		return checkNeg((search_parameter.setcodes.size() && check_set_code(CardSetcodes(data._data), search_parameter.setcodes))
						|| Utils::ContainsSubstring(strings.uppercase_name, search_parameter.tokens)
						|| Utils::ContainsSubstring(strings.uppercase_text, search_parameter.tokens));
	}
}
void DeckBuilder::ClearSearch() {
	mainGame->cbCardType->setSelected(0);
	mainGame->cbCardType2->setSelected(0);
	mainGame->cbCardType2->setEnabled(false);
	mainGame->cbRace->setEnabled(false);
	mainGame->cbAttribute->setEnabled(false);
	mainGame->ebAttack->setEnabled(false);
	mainGame->ebDefense->setEnabled(false);
	mainGame->ebStar->setEnabled(false);
	mainGame->ebScale->setEnabled(false);
	mainGame->ebCardName->setText(L"");
	searched_terms.clear();
	ClearFilter();
	results.clear();
	result_string = L"0";
	scroll_pos = 0;
	search_result_scroll_offset = 0.0f;
	search_result_scroll_velocity = 0.0f;
	search_result_scroll_inertia = false;
	search_result_scroll_pending = false;
	search_result_scroll_active = false;
	mainGame->env->setFocus(mainGame->ebCardName);
}
void DeckBuilder::ClearFilter() {
	mainGame->cbAttribute->setSelected(0);
	mainGame->cbRace->setSelected(0);
	mainGame->cbLimit->setSelected(0);
	mainGame->ebAttack->setText(L"");
	mainGame->ebDefense->setText(L"");
	mainGame->ebStar->setText(L"");
	mainGame->ebScale->setText(L"");
	filter_effect = 0;
	for(int i = 0; i < 32; ++i)
		mainGame->chkCategory[i]->setChecked(false);
	filter_marks = 0;
	for(int i = 0; i < 8; i++)
		mainGame->btnMark[i]->setPressed(false);
}
void DeckBuilder::SortList() {
	auto last = [&] {
		auto it = results.begin(), left = it;
		for(; it != results.end(); ++it) {
			if(searched_terms.find(gDataManager->GetUppercaseName((*it)->code)) != searched_terms.end()) {
				std::iter_swap(left, it);
				++left;
			}
		}
		return left;
	}();
	auto sort = [&](const auto& comparator) {
		std::sort(last, results.end(), comparator);
		std::sort(results.begin(), last, comparator);
	};
	switch(mainGame->cbSortType->getItemData(mainGame->cbSortType->getSelected())) {
	case SORT_MODIFIER::SORT_MODIFIER_LEVEL_DESC:
		sort(DataManager::deck_sort_lv);
		break;
	case SORT_MODIFIER::SORT_MODIFIER_ATK_DESC:
		sort( DataManager::deck_sort_atk);
		break;
	case SORT_MODIFIER::SORT_MODIFIER_DEF_DESC:
		sort(DataManager::deck_sort_def);
		break;
	case SORT_MODIFIER::SORT_MODIFIER_NAME_ASC:
		sort(DataManager::deck_sort_name);
		break;
	case SORT_MODIFIER::SORT_MODIFIER_PASSCODE_DESC:
		sort(DataManager::deck_sort_passcode_descending);
		break;
	case SORT_MODIFIER::SORT_MODIFIER_PASSCODE_ASC:
		sort(std::not_fn(DataManager::deck_sort_passcode_descending));
		break;
	}
}
void DeckBuilder::ClearDeck() {
	current_deck.main.clear();
	current_deck.extra.clear();
	current_deck.side.clear();

	main_and_extra_legend_count_monster = 0;
	main_legend_count_spell = 0;
	main_legend_count_trap = 0;
	main_skill_count = 0;
	main_monster_count = 0;
	main_spell_count = 0;
	main_trap_count = 0;

	extra_fusion_count = 0;
	extra_xyz_count = 0;
	extra_synchro_count = 0;
	extra_link_count = 0;
	extra_rush_ritual_count = 0;

	side_monster_count = 0;
	side_spell_count = 0;
	side_trap_count = 0;
}
void DeckBuilder::RefreshLimitationStatus() {
	main_and_extra_legend_count_monster = DeckManager::CountLegends(current_deck.main, TYPE_MONSTER) + DeckManager::CountLegends(current_deck.extra, TYPE_MONSTER);
	main_legend_count_spell = DeckManager::CountLegends(current_deck.main, TYPE_SPELL);
	main_legend_count_trap = DeckManager::CountLegends(current_deck.main, TYPE_TRAP);
	main_skill_count = DeckManager::TypeCount(current_deck.main, TYPE_SKILL);
	main_monster_count = DeckManager::TypeCount(current_deck.main, TYPE_MONSTER);
	main_spell_count = DeckManager::TypeCount(current_deck.main, TYPE_SPELL);
	main_trap_count = DeckManager::TypeCount(current_deck.main, TYPE_TRAP);

	extra_fusion_count = DeckManager::TypeCount(current_deck.extra, TYPE_FUSION);
	extra_xyz_count = DeckManager::TypeCount(current_deck.extra, TYPE_XYZ);
	extra_synchro_count = DeckManager::TypeCount(current_deck.extra, TYPE_SYNCHRO);
	extra_link_count = DeckManager::TypeCount(current_deck.extra, TYPE_LINK);
	extra_rush_ritual_count = DeckManager::TypeCount(current_deck.extra, TYPE_RITUAL);

	side_monster_count = DeckManager::TypeCount(current_deck.side, TYPE_MONSTER);
	side_spell_count = DeckManager::TypeCount(current_deck.side, TYPE_SPELL);
	side_trap_count = DeckManager::TypeCount(current_deck.side, TYPE_TRAP);
}
void DeckBuilder::RefreshLimitationStatusOnRemoved(const CardDataC* card, DeckType location) {
	switch(location) {
		case DeckType::MAIN:
		{
			if(card->type & TYPE_MONSTER) {
				--main_monster_count;
				if(card->ot & SCOPE_LEGEND)
					--main_and_extra_legend_count_monster;
			}
			if(card->type & TYPE_SPELL) {
				--main_spell_count;
				if(card->ot & SCOPE_LEGEND)
					--main_legend_count_spell;
			}
			if(card->type & TYPE_TRAP) {
				--main_trap_count;
				if(card->ot & SCOPE_LEGEND)
					--main_legend_count_trap;
			}
			if(card->type & TYPE_SKILL)
				--main_skill_count;
			break;
		}
		case DeckType::EXTRA:
		{
			if(card->ot & SCOPE_LEGEND)
				--main_and_extra_legend_count_monster;
			if(card->type & TYPE_FUSION)
				--extra_fusion_count;
			if(card->type & TYPE_XYZ)
				--extra_xyz_count;
			if(card->type & TYPE_SYNCHRO)
				--extra_synchro_count;
			if(card->type & TYPE_LINK)
				--extra_link_count;
			if(card->type & TYPE_RITUAL)
				--extra_rush_ritual_count;
			break;
		}
		case DeckType::SIDE:
		{
			if(card->type & TYPE_MONSTER)
				--side_monster_count;
			if(card->type & TYPE_SPELL)
				--side_spell_count;
			if(card->type & TYPE_TRAP)
				--side_trap_count;
			break;
		}
	}
}
void DeckBuilder::RefreshLimitationStatusOnAdded(const CardDataC* card, DeckType location) {
	switch(location) {
		case DeckType::MAIN:
		{
			if(card->type & TYPE_MONSTER) {
				++main_monster_count;
				if(card->ot & SCOPE_LEGEND)
					++main_and_extra_legend_count_monster;
			}
			if(card->type & TYPE_SPELL) {
				++main_spell_count;
				if(card->ot & SCOPE_LEGEND)
					++main_legend_count_spell;
			}
			if(card->type & TYPE_TRAP) {
				++main_trap_count;
				if(card->ot & SCOPE_LEGEND)
					++main_legend_count_trap;
			}
			if(card->type & TYPE_SKILL)
				++main_skill_count;
			break;
		}
		case DeckType::EXTRA:
		{
			if(card->ot & SCOPE_LEGEND)
				++main_and_extra_legend_count_monster;
			if(card->type & TYPE_FUSION)
				++extra_fusion_count;
			if(card->type & TYPE_XYZ)
				++extra_xyz_count;
			if(card->type & TYPE_SYNCHRO)
				++extra_synchro_count;
			if(card->type & TYPE_LINK)
				++extra_link_count;
			if(card->type & TYPE_RITUAL)
				++extra_rush_ritual_count;
			break;
		}
		case DeckType::SIDE:
		{
			if(card->type & TYPE_MONSTER)
				++side_monster_count;
			if(card->type & TYPE_SPELL)
				++side_spell_count;
			if(card->type & TYPE_TRAP)
				++side_trap_count;
			break;
		}
	}
}
bool DeckBuilder::push_main(const CardDataC* pointer, int seq, bool forced) {
	if(pointer->isRitualMonster()) {
		if(mainGame->is_siding) {
			if(mainGame->dInfo.HasFieldFlag(DUEL_EXTRA_DECK_RITUAL))
				return false;
		} else if(pointer->isRush() && !forced)
			return false;
	}
	if(pointer->type & (TYPE_FUSION | TYPE_SYNCHRO | TYPE_XYZ))
		return false;
	if((pointer->type & (TYPE_LINK | TYPE_SPELL)) == TYPE_LINK)
		return false;
	auto& container = current_deck.main;
	if(!forced && !mainGame->is_siding) {
		if(main_and_extra_legend_count_monster >= 1 && (pointer->ot & SCOPE_LEGEND) && (pointer->type & TYPE_MONSTER))
			return false;
		if(main_legend_count_spell >= 1 && (pointer->ot & SCOPE_LEGEND) && (pointer->type & TYPE_SPELL))
			return false;
		if(main_legend_count_trap >= 1 && (pointer->ot & SCOPE_LEGEND) && (pointer->type & TYPE_TRAP))
			return false;
		if(main_skill_count >= 1 && (pointer->type & TYPE_SKILL))
			return false;
		if(container.size() >= 60)
			return false;
	}
	if(seq >= 0 && seq < (int)container.size())
		container.insert(container.begin() + seq, pointer);
	else
		container.push_back(pointer);
	GetHoveredCard();
	RefreshLimitationStatusOnAdded(pointer, DeckType::MAIN);
	return true;
}
bool DeckBuilder::push_extra(const CardDataC* pointer, int seq, bool forced) {
	if(pointer->isRitualMonster()) {
		if(mainGame->is_siding) {
			if(!mainGame->dInfo.HasFieldFlag(DUEL_EXTRA_DECK_RITUAL))
				return false;
		} else if(!pointer->isRush() && !forced)
			return false;
	} else if(pointer->type & TYPE_LINK) {
		if(pointer->type & TYPE_SPELL)
			return false;
	} else if((pointer->type & (TYPE_FUSION | TYPE_SYNCHRO | TYPE_XYZ)) == 0)
		return false;
	auto& container = current_deck.extra;
	if(!forced && !mainGame->is_siding) {
		if(main_and_extra_legend_count_monster >= 1 && (pointer->ot & SCOPE_LEGEND))
			return false;
		if(container.size() >= 15)
			return false;
	}
	if(seq >= 0 && seq < (int)container.size())
		container.insert(container.begin() + seq, pointer);
	else
		container.push_back(pointer);
	GetHoveredCard();
	RefreshLimitationStatusOnAdded(pointer, DeckType::EXTRA);
	return true;
}
bool DeckBuilder::push_side(const CardDataC* pointer, int seq, bool forced) {
	auto& container = current_deck.side;
	if(!mainGame->is_siding && !forced && container.size() >= 15)
		return false;
	if(seq >= 0 && seq < (int)container.size())
		container.insert(container.begin() + seq, pointer);
	else
		container.push_back(pointer);
	GetHoveredCard();
	RefreshLimitationStatusOnAdded(pointer, DeckType::SIDE);
	return true;
}
void DeckBuilder::pop_main(int seq) {
	auto& container = current_deck.main;
	auto it = container.begin() + seq;
	auto pcard = *it;
	container.erase(it);
	GetHoveredCard();
	RefreshLimitationStatusOnRemoved(pcard, DeckType::MAIN);
}
void DeckBuilder::pop_extra(int seq) {
	auto& container = current_deck.extra;
	auto it = container.begin() + seq;
	auto pcard = *it;
	container.erase(it);
	GetHoveredCard();
	RefreshLimitationStatusOnRemoved(pcard, DeckType::EXTRA);
}
void DeckBuilder::pop_side(int seq) {
	auto& container = current_deck.side;
	auto it = container.begin() + seq;
	auto pcard = *it;
	container.erase(it);
	GetHoveredCard();
	RefreshLimitationStatusOnRemoved(pcard, DeckType::SIDE);
}
bool DeckBuilder::check_limit(const CardDataC* pointer) {
	uint32_t limitcode = pointer->alias ? pointer->alias : pointer->code;
	int found = 0;
	int limit = filterList->whitelist ? 0 : 3;
	auto endit = filterList->content.end();
	auto it = filterList->GetLimitationIterator(pointer);
	if(it != endit)
		limit = it->second;
	if(limit == 0)
		return false;
	const auto& deck = current_deck;
	for(auto* plist : { &deck.main, &deck.extra, &deck.side }) {
		for(auto& pcard : *plist) {
			if(pcard->code == limitcode || pcard->alias == limitcode) {
				if((it = filterList->content.find(pcard->code)) != endit)
					limit = std::min(limit, it->second);
				else if((it = filterList->content.find(pcard->alias)) != endit)
					limit = std::min(limit, it->second);
				found++;
			}
			if(limit <= found)
				return false;
		}
	}
	return true;
}
void DeckBuilder::RefreshCurrentDeck() {
	DeckManager::RefreshDeck(current_deck);
	RefreshLimitationStatus();
}
}
