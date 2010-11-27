/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include "frame_processor_device.h"

#include "../format/video_format.h"

#include <boost/noncopyable.hpp>

#include <memory>

namespace caspar { namespace core {

class frame_renderer :  boost::noncopyable
{
public:
	frame_renderer(frame_processor_device& frame_processor, const video_format_desc& format_desc_);
		
	internal_frame_ptr render(const internal_frame_ptr& frames);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<frame_renderer> frame_renderer_ptr;

}}