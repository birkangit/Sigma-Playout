#include "../../stdafx.h"

#include "ffmpeg_producer.h"

#include "input.h"
#include "audio/audio_decoder.h"
#include "video/video_decoder.h"

#include "../../format/video_format.h"
#include "../../processor/draw_frame.h"

#include <tbb/parallel_invoke.h>

#include <deque>

using namespace boost::assign;

namespace caspar { namespace core { namespace ffmpeg{
	
struct ffmpeg_producer : public frame_producer
{
	input								input_;			
	audio_decoder						audio_decoder_;
	video_decoder						video_decoder_;

	std::deque<safe_ptr<write_frame>>	video_frame_channel_;	
	std::deque<std::vector<short>>		audio_chunk_channel_;

	std::queue<safe_ptr<draw_frame>>	ouput_channel_;
	
	const std::wstring					filename_;
	
	safe_ptr<draw_frame>				last_frame_;

	video_format_desc					format_desc_;

public:
	explicit ffmpeg_producer(const std::wstring& filename, const  std::vector<std::wstring>& params) 
		: filename_(filename)
		, last_frame_(draw_frame(draw_frame::empty()))
		, input_(filename)
		, video_decoder_(input_.get_video_codec_context().get())
		, audio_decoder_(input_.get_audio_codec_context().get(), input_.fps())
	{				
		input_.set_loop(std::find(params.begin(), params.end(), L"LOOP") != params.end());

		auto seek = std::find(params.begin(), params.end(), L"SEEK");
		if(seek != params.end() && ++seek != params.end())
		{
			if(!input_.seek(boost::lexical_cast<unsigned long long>(*seek)))
				CASPAR_LOG(warning) << "Failed to seek file: " << filename_  << "to frame" << *seek;
		}
	}

	virtual void initialize(const safe_ptr<frame_processor_device>& frame_processor)
	{
		format_desc_ = frame_processor->get_video_format_desc();
		video_decoder_.initialize(frame_processor);
	}
		
	virtual safe_ptr<draw_frame> receive()
	{
		while(ouput_channel_.empty() && !input_.is_eof())
		{	
			aligned_buffer video_packet;
			if(video_frame_channel_.size() < 3)	
				video_packet = input_.get_video_packet();		
			
			aligned_buffer audio_packet;
			if(audio_chunk_channel_.size() < 3)	
				audio_packet = input_.get_audio_packet();		

			tbb::parallel_invoke(
			[&]
			{ // Video Decoding and Scaling
				if(!video_packet.empty())
				{
					auto frame = video_decoder_.execute(video_packet);
					video_frame_channel_.push_back(std::move(frame));	
				}
			}, 
			[&] 
			{ // Audio Decoding
				if(!audio_packet.empty())
				{
					auto chunks = audio_decoder_.execute(audio_packet);
					audio_chunk_channel_.insert(audio_chunk_channel_.end(), chunks.begin(), chunks.end());
				}
			});

			while(!video_frame_channel_.empty() && (!audio_chunk_channel_.empty() || input_.get_audio_codec_context() == nullptr))
			{
				if(input_.get_audio_codec_context() != nullptr) 
				{
					video_frame_channel_.front()->audio_data() = std::move(audio_chunk_channel_.front());
					audio_chunk_channel_.pop_front();
				}
							
				ouput_channel_.push(video_frame_channel_.front());
				video_frame_channel_.pop_front();
			}				

			if(ouput_channel_.empty() && video_packet.empty() && audio_packet.empty())			
				return last_frame_;			
		}

		auto result = last_frame_;
		if(!ouput_channel_.empty())
		{
			result = std::move(ouput_channel_.front());
			last_frame_ = draw_frame(result);
			last_frame_->audio_volume(0.0); // last_frame should not have audio
			ouput_channel_.pop();
		}
		else if(input_.is_eof())
			return draw_frame::eof();

		return result;
	}

	virtual std::wstring print() const
	{
		return L"ffmpeg[" + boost::filesystem::wpath(filename_).filename() + L"]";
	}
};

safe_ptr<frame_producer> create_ffmpeg_producer(const std::vector<std::wstring>& params)
{			
	static const std::vector<std::wstring> extensions = list_of(L"mpg")(L"avi")(L"mov")(L"dv")(L"wav")(L"mp3")(L"mp4")(L"f4v")(L"flv");
	std::wstring filename = params[0];
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::wstring& ex) -> bool
		{					
			return boost::filesystem::is_regular_file(boost::filesystem::wpath(filename).replace_extension(ex));
		});

	if(ext == extensions.end())
		return frame_producer::empty();

	return make_safe<ffmpeg_producer>(filename + L"." + *ext, params);
}

}}}