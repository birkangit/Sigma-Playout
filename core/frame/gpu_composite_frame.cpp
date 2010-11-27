#include "../StdAfx.h"

#include "gpu_composite_frame.h"
#include "../../common/gl/utility.h"
#include "../../common/utility/memory.h"

#include <boost/range/algorithm.hpp>

#include <algorithm>
#include <numeric>

#include <tbb/parallel_for.h>

namespace caspar { namespace core {
	
struct gpu_composite_frame::implementation : boost::noncopyable
{
	implementation(gpu_composite_frame* self) : self_(self){}

	void begin_write()
	{
		boost::range::for_each(frames_, std::mem_fn(&gpu_frame::begin_write));		
	}

	void end_write()
	{
		boost::range::for_each(frames_, std::mem_fn(&gpu_frame::end_write));				
	}
	
	void begin_read()
	{	
		boost::range::for_each(frames_, std::mem_fn(&gpu_frame::begin_read));		
	}

	void end_read()
	{
		boost::range::for_each(frames_, std::mem_fn(&gpu_frame::end_read));	
	}

	void draw(const gpu_frame_shader_ptr& shader)
	{
		glPushMatrix();
		glTranslated(self_->x()*2.0, self_->y()*2.0, 0.0);
		boost::range::for_each(frames_, std::bind(&gpu_frame::draw, std::placeholders::_1, shader));
		glPopMatrix();
	}
		
	void add(const gpu_frame_ptr& frame)
	{
		if(frame == nullptr || frame == gpu_frame::null())
			return;

		frames_.push_back(frame);

		if(self_->audio_data().empty())
			self_->audio_data() = std::move(frame->audio_data());
		else
		{
			tbb::parallel_for
			(
				tbb::blocked_range<size_t>(0, frame->audio_data().size()),
				[&](const tbb::blocked_range<size_t>& r)
				{
					for(size_t n = r.begin(); n < r.end(); ++n)
					{
						self_->audio_data()[n] = static_cast<short>(
							static_cast<int>(self_->audio_data()[n]) + 
							static_cast<int>(frame->audio_data()[n]) & 0xFFFF);	
					}
				}
			);
		}
	}

	unsigned char* data(size_t)
	{
		BOOST_THROW_EXCEPTION(invalid_operation());
	}

	gpu_composite_frame* self_;
	std::vector<gpu_frame_ptr> frames_;
	size_t size_;
};

#if defined(_MSC_VER)
#pragma warning (disable : 4355) // 'this' : used in base member initializer list
#endif

gpu_composite_frame::gpu_composite_frame() 
	: gpu_frame(0, 0), impl_(new implementation(this)){}
void gpu_composite_frame::begin_write(){impl_->begin_write();}
void gpu_composite_frame::end_write(){impl_->end_write();}	
void gpu_composite_frame::begin_read(){impl_->begin_read();}
void gpu_composite_frame::end_read(){impl_->end_read();}
void gpu_composite_frame::draw(const gpu_frame_shader_ptr& shader){impl_->draw(shader);}
unsigned char* gpu_composite_frame::data(size_t index){return impl_->data(index);}
void gpu_composite_frame::add(const gpu_frame_ptr& frame){impl_->add(frame);}

gpu_frame_ptr gpu_composite_frame::interlace(const gpu_frame_ptr& frame1,
												const gpu_frame_ptr& frame2, 
												video_mode mode)
{			
	auto result = std::make_shared<gpu_composite_frame>();
	result->add(frame1);
	result->add(frame2);
	if(mode == video_mode::upper)
	{
		frame1->mode(video_mode::upper);
		frame2->mode(video_mode::lower);
	}
	else
	{
		frame1->mode(video_mode::lower);
		frame2->mode(video_mode::upper);
	}
	return result;
}

}}