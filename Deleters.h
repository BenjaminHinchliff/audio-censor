#pragma once

#include <pocketsphinx.h>
#include <iostream>

namespace Deleters {
	struct CmdLn
	{
		void operator()(cmd_ln_t *config);
	};
	
	struct PsDecoder
	{
		void operator()(ps_decoder_t *ps);
	};

	struct PsSeg
	{
		void operator()(ps_seg_t *seg);
	};
}
