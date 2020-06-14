#include "Deleters.h"

namespace Deleters {
	void CmdLn::operator()(cmd_ln_t *config)
	{
		cmd_ln_free_r(config);
	}

	void PsDecoder::operator()(ps_decoder_t *ps)
	{
		ps_free(ps);
	}

	void PsSeg::operator()(ps_seg_t *seg)
	{
		ps_seg_free(seg);
	}
}
