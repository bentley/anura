/*
	Copyright (C) 2003-2014 by David White <davewx7@gmail.com>
	
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	   1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgement in the product documentation would be
	   appreciated but is not required.

	   2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.

	   3. This notice may not be removed or altered from any source
	   distribution.
*/

#pragma once

#include <boost/shared_ptr.hpp>

#include "scrollbar_widget.hpp"
#include "widget.hpp"

namespace gui 
{
	class ScrollableWidget : public Widget
	{
	public:
		ScrollableWidget();
		ScrollableWidget(const variant& v, game_logic::FormulaCallable* e);
		void set_yscroll(int yscroll);
		virtual void setDim(int w, int h);

		virtual void handleDraw() const override;
		virtual bool handleEvent(const SDL_Event& event, bool claimed) override;

		virtual void setLoc(int x, int y);
	protected:
		~ScrollableWidget();
		void set_virtual_height(int height);
		void set_scroll_step(int step);
		void set_arrow_scroll_step(int step);
		void update_scrollbar();

		int yscroll() const { return yscroll_; }
		int virtual_height() const { return virtual_height_; }

		virtual void setValue(const std::string& key, const variant& v);
		virtual variant getValue(const std::string& key) const;
	private:
		virtual void onSetYscroll(int old_yscroll, int new_yscroll);

		int yscroll_;
		int virtual_height_;
		int step_;
		int arrow_step_;

		bool auto_scroll_bottom_;

		ScrollbarWidgetPtr scrollbar_;
	};

	typedef boost::intrusive_ptr<ScrollableWidget> ScrollableWidgetPtr;
	typedef boost::intrusive_ptr<const ScrollableWidget> ConstScrollableWidgetPtr;

}
