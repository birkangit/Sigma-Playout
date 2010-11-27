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
#include "../../stdafx.h"

#include "transition_producer.h"

#include "../../format/video_format.h"
#include "../../processor/frame.h"
#include "../../processor/composite_frame.h"
#include "../../processor/frame_processor_device.h"

#include "../../producer/frame_producer_device.h"

#include <boost/range/algorithm/copy.hpp>

namespace caspar { namespace core {	

struct transition_producer::implementation : boost::noncopyable
{
	implementation(const frame_producer_ptr& dest, const transition_info& info) 
		: current_frame_(0), info_(info), dest_producer_(dest)
	{
		if(!dest)
			BOOST_THROW_EXCEPTION(null_argument() << arg_name_info("dest"));
	}
		
	frame_producer_ptr get_following_producer() const
	{
		return dest_producer_;
	}
	
	void set_leading_producer(const frame_producer_ptr& producer)
	{
		source_producer_ = producer;
	}
		
	frame_ptr render_frame()
	{
		if(current_frame_ == 0)
			CASPAR_LOG(info) << "Transition started.";

		frame_ptr result = [&]() -> frame_ptr
		{
			if(current_frame_++ >= info_.duration)
				return nullptr;

			frame_ptr source;
			frame_ptr dest;

			tbb::parallel_invoke
			(
				[&]{dest   = render_frame(dest_producer_);},
				[&]{source = render_frame(source_producer_);}
			);

			return compose(dest, source);
		}();

		if(result == nullptr)
			CASPAR_LOG(info) << "Transition ended.";

		return result;
	}

	frame_ptr render_frame(frame_producer_ptr& producer)
	{
		if(producer == nullptr)
			return nullptr;

		frame_ptr frame;
		try
		{
			frame = producer->render_frame();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			producer = nullptr;
			CASPAR_LOG(warning) << "Removed producer from transition.";
		}

		if(frame == nullptr)
		{
			if(producer == nullptr || producer->get_following_producer() == nullptr)
				return nullptr;

			try
			{
				auto following = producer->get_following_producer();
				following->initialize(frame_processor_);
				following->set_leading_producer(producer);
				producer = following;
			}
			catch(...)
			{
				CASPAR_LOG(warning) << "Failed to initialize following producer. Removing it.";
				producer = nullptr;
			}

			return render_frame(producer);
		}
		return frame;
	}
			
	void set_volume(const frame_ptr& frame, int volume)
	{
		assert(volume >= 0 && volume <= 256);
		std::for_each(frame->get_audio_data().begin(), frame->get_audio_data().end(), [&](int16_t& sample)
		{
			sample = static_cast<short>((static_cast<int>(sample)*volume)>>8);
		});
	}
		
	frame_ptr compose(frame_ptr dest_frame, frame_ptr src_frame) 
	{	
		if(!dest_frame && !src_frame)
			return nullptr;

		if(info_.type == transition::cut)		
			return src_frame;

		dest_frame = dest_frame ? dest_frame : frame::empty();
		src_frame  = src_frame  ? src_frame  : frame::empty();
								
		double alpha = static_cast<double>(current_frame_)/static_cast<double>(info_.duration);
		int volume = static_cast<int>(alpha*256.0);
				
		tbb::parallel_invoke
		(
			[&]{set_volume(dest_frame, volume);},
			[&]{set_volume(src_frame, 256-volume);}
		);

		double dir = info_.direction == transition_direction::from_left ? 1.0 : -1.0;		
		
		if(info_.type == transition::mix)
			dest_frame->get_render_transform().alpha = alpha;		
		else if(info_.type == transition::slide)			
			dest_frame->get_render_transform().pos = boost::make_tuple((-1.0+alpha)*dir, 0.0);			
		else if(info_.type == transition::push)
		{
			dest_frame->get_render_transform().pos = boost::make_tuple((-1.0+alpha)*dir, 0.0);
			src_frame->get_render_transform().pos = boost::make_tuple((0.0+alpha)*dir, 0.0);
		}
		else if(info_.type == transition::wipe)
		{
			dest_frame->get_render_transform().pos = boost::make_tuple((-1.0+alpha)*dir, 0.0);			
			dest_frame->get_render_transform().uv = boost::make_tuple((-1.0+alpha)*dir, 1.0, 1.0-(1.0-alpha)*dir, 0.0);				
		}
						
		return std::make_shared<composite_frame>(src_frame, dest_frame);
	}
		
	void initialize(const frame_processor_device_ptr& frame_processor)
	{
		dest_producer_->initialize(frame_processor);
		frame_processor_ = frame_processor;
	}
	
	frame_producer_ptr			source_producer_;
	frame_producer_ptr			dest_producer_;
	
	unsigned short				current_frame_;
	
	const transition_info		info_;
	frame_processor_device_ptr	frame_processor_;
};

transition_producer::transition_producer(const frame_producer_ptr& dest, const transition_info& info) : impl_(new implementation(dest, info)){}
frame_ptr transition_producer::render_frame(){return impl_->render_frame();}
frame_producer_ptr transition_producer::get_following_producer() const{return impl_->get_following_producer();}
void transition_producer::set_leading_producer(const frame_producer_ptr& producer) { impl_->set_leading_producer(producer); }
void transition_producer::initialize(const frame_processor_device_ptr& frame_processor) { impl_->initialize(frame_processor);}

}}
