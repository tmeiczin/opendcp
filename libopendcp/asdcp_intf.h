/* */
#ifndef _OPENDCP_ASDCP_H_
#define _OPENDCP_ASDCP_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ASDCP::WriterInfo    info;
    ASDCP::AESEncContext *aes_context;
    ASDCP::HMACContext   *hmac_context;
} writer_info_t;

typedef struct {
    ASDCP::JP2K::CodestreamParser   parser;
    ASDCP::JP2K::MXFWriter          mxf_writer;
    ASDCP::JP2K::PictureDescriptor  picture_desc;
    writer_info_t                   writer_info;
} j2k_context_t;

typedef struct {
    ASDCP::JP2K::CodestreamParser   parser;
    ASDCP::JP2K::MXFSWriter         mxf_writer;
    ASDCP::JP2K::PictureDescriptor  picture_desc;
    writer_info_t                   writer_info;
} j2k_s_context_t;

typedef struct {
    ASDCP::PCM::WAVParser           parser;
    ASDCP::PCM::AudioDescriptor     audio_desc;
    ASDCP::PCM::MXFWriter           mxf_writer;
    writer_info_t                   writer_info;
    ASDCP::PCM::AudioDescriptor     audio_desc_channel[MAX_AUDIO_CHANNELS];
    ASDCP::PCM::WAVParser           parser_channel[MAX_AUDIO_CHANNELS];
} pcm_context_t;

#ifdef __cplusplus
}
#endif

#endif  //_OPENDCP_ASDCP_H_
