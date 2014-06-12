/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef NO_EDITOR
#include <vector>

#include "asserts.hpp"
#include "controls.hpp"
#include "image_widget.hpp"
#include "joystick.hpp"
#include "dropdown_widget.hpp"
#include "foreach.hpp"
#include "kre/Geometry.hpp"
#include "graphics.hpp"
#include "input.hpp"
#include "raster.hpp"

namespace gui {

namespace {
	const std::string dropdown_button_image = "dropdown_button";
}

dropdown_widget::dropdown_widget(const dropdown_list& list, int width, int height, dropdown_type type)
	: list_(list), type_(type), current_selection_(0), dropdown_height_(100)
{
	setEnvironment();
	setDim(width, height);
	editor_ = new TextEditorWidget(width, height);
	editor_->setOnUserChangeHandler(std::bind(&dropdown_widget::textChange, this));
	editor_->setOnEnterHandler(std::bind(&dropdown_widget::text_enter, this));
	editor_->setOnTabHandler(std::bind(&dropdown_widget::text_enter, this));
	dropdown_image_ = WidgetPtr(new GuiSectionWidget(dropdown_button_image));
	//if(type_ == DROPDOWN_COMBOBOX) {
	//	editor_->setFocus(true);
	//}
	setZOrder(1);

	init();
}

dropdown_widget::dropdown_widget(const variant& v, game_logic::FormulaCallable* e)
	: widget(v,e), current_selection_(0), dropdown_height_(100)
{
	ASSERT_LOG(getEnvironment() != 0, "You must specify a callable environment");
	if(v.has_key("type")) {
		std::string s = v["type"].as_string();
		if(s == "combo" || s == "combobox") {
			type_ = DROPDOWN_COMBOBOX;
		} else if(s == "list" || s == "listbox") {
			type_ = DROPDOWN_LIST;
		} else {
			ASSERT_LOG(false, "Unreognised type: " << s);
		}
	}
	if(v.has_key("text_edit")) {
		editor_ = new TextEditorWidget(v["text_edit"], e);
	} else {
		editor_ = new TextEditorWidget(width(), height());
	}
	editor_->setOnEnterHandler(std::bind(&dropdown_widget::text_enter, this));
	editor_->setOnTabHandler(std::bind(&dropdown_widget::text_enter, this));
	editor_->setOnUserChangeHandler(std::bind(&dropdown_widget::textChange, this));
	if(v.has_key("on_change")) {
		change_handler_ = getEnvironment()->createFormula(v["on_change"]);
		on_change_ = std::bind(&dropdown_widget::changeDelegate, this, _1);
	}
	if(v.has_key("on_select")) {
		select_handler_ = getEnvironment()->createFormula(v["on_select"]);
		on_select_ = std::bind(&dropdown_widget::selectDelegate, this, _1, _2);
	}
	if(v.has_key("item_list")) {
		list_ = v["item_list"].as_list_string();
	}
	if(v.has_key("default")) {
		current_selection_ = v["default"].as_int();
	}
	init();
}

void dropdown_widget::init()
{
	const int dropdown_image_size = std::max(height(), dropdown_image_->height());
	label_ = new label(list_.size() > 0 ? list_[current_selection_] : "No items");
	label_->setLoc(0, (height() - label_->height()) / 2);
	dropdown_image_->setLoc(width() - height() + (height() - dropdown_image_->width()) / 2, 
		(height() - dropdown_image_->height()) / 2);
	// go on ask me why there is a +20 in the line below.
	// because TextEditorWidget uses a magic -20 when setting the width!
	// The magic +4's are because we want the rectangles drawn around the TextEditorWidget 
	// to match the ones we draw around the dropdown image.
	editor_->setDim(width() - dropdown_image_size + 20 + 4, dropdown_image_size + 4);
	editor_->setLoc(-2, -2);

	if(dropdown_menu_) {
		dropdown_menu_.reset(new grid(1));
	} else {
		dropdown_menu_ = new grid(1);
	}
	dropdown_menu_->setLoc(0, height()+2);
	dropdown_menu_->allow_selection(true);
	dropdown_menu_->set_show_background(true);
	dropdown_menu_->swallow_clicks(true);
	dropdown_menu_->set_col_width(0, width());
	dropdown_menu_->set_max_height(dropdown_height_);
	dropdown_menu_->setDim(width(), dropdown_height_);
	dropdown_menu_->must_select();
	foreach(const std::string& s, list_) {
		dropdown_menu_->add_col(WidgetPtr(new label(s, graphics::color_white())));
	}
	dropdown_menu_->register_selection_callback(std::bind(&dropdown_widget::execute_selection, this, _1));
	dropdown_menu_->setVisible(false);

}

void dropdown_widget::setSelection(int selection)
{
	if(selection >= 0 || size_t(selection) < list_.size()) {
		current_selection_ = selection;
		if(type_ == DROPDOWN_LIST) {
			label_->setText(list_[current_selection_]);
		} else if(type_ == DROPDOWN_COMBOBOX) {
			editor_->setText(list_[current_selection_]);
		}
	}
}

void dropdown_widget::changeDelegate(const std::string& s)
{
	if(getEnvironment()) {
		game_logic::MapFormulaCallable* callable = new game_logic::MapFormulaCallable(getEnvironment());
		callable->add("selection", variant(s));
		variant v(callable);
		variant value = change_handler_->execute(*callable);
		getEnvironment()->createFormula(value);
	} else {
		std::cerr << "dropdown_widget::changeDelegate() called without environment!" << std::endl;
	}
}

void dropdown_widget::selectDelegate(int selection, const std::string& s)
{
	if(getEnvironment()) {
		game_logic::MapFormulaCallable* callable = new game_logic::MapFormulaCallable(getEnvironment());
		if(selection == -1) {
			callable->add("selection", variant(selection));
		} else {
			callable->add("selection", variant(s));
		}
		variant v(callable);
		variant value = select_handler_->execute(*callable);
		getEnvironment()->createFormula(value);
	} else {
		std::cerr << "dropdown_widget::selectDelegate() called without environment!" << std::endl;
	}
}

void dropdown_widget::text_enter()
{
	dropdown_list::iterator it = std::find(list_.begin(), list_.end(), editor_->text());
	if(it == list_.end()) {
		current_selection_ = -1;
	} else {
		current_selection_ = it - list_.begin();
	}
	if(on_select_) {
		on_select_(current_selection_, editor_->text());
	}
}

void dropdown_widget::textChange()
{
	if(on_change_) {
		on_change_(editor_->text());
	}
}

void dropdown_widget::handleDraw() const
{
	if(type_ == DROPDOWN_LIST) {
		graphics::draw_hollow_rect(
			rect(x()-1, y()-1, width()+2, height()+2).sdl_rect(), 
			hasFocus() ? graphics::color_white() : graphics::color_grey());
	}
	graphics::draw_hollow_rect(
		rect(x()+width()-height(), y()-1, height()+1, height()+2).sdl_rect(), 
		hasFocus() ? graphics::color_white() : graphics::color_grey());

	glPushMatrix();
	glTranslatef(GLfloat(x() & ~1), GLfloat(y() & ~1), 0.0);
	if(type_ == DROPDOWN_LIST) {
		label_->handleDraw();
	} else if(type_ == DROPDOWN_COMBOBOX) {
		editor_->handleDraw();
	}
	if(dropdown_image_) {
		dropdown_image_->draw();
	}
	if(dropdown_menu_ && dropdown_menu_->visible()) {
		dropdown_menu_->draw();
	}
	glPopMatrix();
}

void dropdown_widget::handleProcess()
{
	/*if(hasFocus() && dropdown_menu_) {
		if(joystick::button(0) || joystick::button(1) || joystick::button(2)) {

		}

		if(dropdown_menu_->visible()) {
		} else {
		}
	}*/
}

bool dropdown_widget::handleEvent(const SDL_Event& event, bool claimed)
{
	SDL_Event ev = event;
	switch(ev.type) {
		case SDL_MOUSEMOTION: {
			ev.motion.x -= x() & ~1;
			ev.motion.y -= y() & ~1;
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: {
			ev.button.x -= x() & ~1;
			ev.button.y -= y() & ~1;
			break;
		}
	}

	if(claimed) {
		return claimed;
	}

	if(type_ == DROPDOWN_COMBOBOX && editor_) {
		if(editor_->handleEvent(ev, claimed)) {
			return true;
		}
	}

	if(dropdown_menu_ && dropdown_menu_->visible()) {
		if(dropdown_menu_->handleEvent(ev, claimed)) {
			return true;
		}
	}

	if(hasFocus() && dropdown_menu_) {
		if(event.type == SDL_KEYDOWN 
			&& (ev.key.keysym.sym == controls::get_keycode(controls::CONTROL_ATTACK) 
			|| ev.key.keysym.sym == controls::get_keycode(controls::CONTROL_JUMP))) {
			claimed = true;
			dropdown_menu_->setVisible(!dropdown_menu_->visible());
		}
	}

	if(event.type == SDL_MOUSEMOTION) {
		return handleMouseMotion(event.motion, claimed);
	} else if(event.type == SDL_MOUSEBUTTONDOWN) {
		return handleMousedown(event.button, claimed);
	} else if(event.type == SDL_MOUSEBUTTONUP) {
		return handleMouseup(event.button, claimed);
	}
	return claimed;
}

bool dropdown_widget::handleMousedown(const SDL_MouseButtonEvent& event, bool claimed)
{
	point p(event.x, event.y);
	//int button_state = input::sdl_get_mouse_state(&p.x, &p.y);
	if(pointInRect(p, rect(x(), y(), width()+height(), height()))) {
		claimed = claimMouseEvents();
		if(dropdown_menu_) {
			dropdown_menu_->setVisible(!dropdown_menu_->visible());
		}
	}
	return claimed;
}

void dropdown_widget::set_dropdown_height(int h)
{
	dropdown_height_ = h;
	if(dropdown_menu_) {
		dropdown_menu_->set_max_height(dropdown_height_);
	}
}

bool dropdown_widget::handleMouseup(const SDL_MouseButtonEvent& event, bool claimed)
{
	point p(event.x, event.y);
	//int button_state = input::sdl_get_mouse_state(&p.x, &p.y);
	if(pointInRect(p, rect(x(), y(), width()+height(), height()))) {
		claimed = claimMouseEvents();
	}
	return claimed;
}

bool dropdown_widget::handleMouseMotion(const SDL_MouseMotionEvent& event, bool claimed)
{
	point p;
	int button_state = input::sdl_get_mouse_state(&p.x, &p.y);
	return claimed;
}

void dropdown_widget::execute_selection(int selection)
{
	if(dropdown_menu_) {
		dropdown_menu_->setVisible(false);
	}
	if(selection < 0 || size_t(selection) >= list_.size()) {
		return;
	}
	//std::cerr << "execute_selection: " << selection << std::endl;
	current_selection_ = selection;
	if(type_ == DROPDOWN_LIST) {
		label_->setText(list_[current_selection_]);
	} else if(type_ == DROPDOWN_COMBOBOX) {
		editor_->setText(list_[current_selection_]);
	}
	if(on_select_) {
		if(type_ == DROPDOWN_LIST) {
			on_select_(current_selection_, list_[current_selection_]);
		} else if(type_ == DROPDOWN_COMBOBOX) {
			on_select_(current_selection_, editor_->text());
		}
	}
}

int dropdown_widget::get_max_height() const
{
	// Maximum height required, including dropdown and borders.
	return height() + (dropdown_menu_ ? dropdown_menu_->height() : dropdown_height_) + 2;
}

void dropdown_widget::setValue(const std::string& key, const variant& v)
{
	if(key == "on_change") {
		on_change_ = std::bind(&dropdown_widget::changeDelegate, this, _1);
		change_handler_ = getEnvironment()->createFormula(v);
	} else if(key == "on_select") {
		on_select_ = std::bind(&dropdown_widget::selectDelegate, this, _1, _2);
		select_handler_ = getEnvironment()->createFormula(v);
	} else if(key == "item_list") {
		list_ = v.as_list_string();
		current_selection_ = 0;
	} else if(key == "selection") {
		current_selection_ = v.as_int();
	} else if(key == "type") {
		std::string s = v.as_string();
		if(s == "combo" || s == "combobox") {
			type_ = DROPDOWN_COMBOBOX;
		} else if(s == "list" || s == "listbox") {
			type_ = DROPDOWN_LIST;
		} else {
			ASSERT_LOG(false, "Unreognised type: " << s);
		}
	}
	widget::setValue(key, v);
}

variant dropdown_widget::getValue(const std::string& key) const
{
	if(key == "selection") {
		return variant(current_selection_);
	} else if(key == "selected_item") {
		if(current_selection_ < 0 || size_t(current_selection_) > list_.size()) {
			return variant();
		}
		return variant(list_[current_selection_]);
	}
	return widget::getValue(key);
}

}
#endif
