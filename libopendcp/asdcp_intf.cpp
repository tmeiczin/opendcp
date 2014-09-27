/*
    OpenDCP: Builds Digital Cinema Packages
    Copyright (c) 2010-2013 Terrence Meiczinger, All Rights Reserved

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <AS_DCP.h>
#include <KM_fileio.h>
#include <KM_prng.h>
#include <KM_memio.h>
#include <KM_util.h>
#include <WavFileWriter.h>
#include <iostream>
#include <assert.h>
#include "opendcp.h"
#include <asdcp_intf.h>

using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 4 * Kumu::Megabyte;

int write_j2k_mxf(opendcp_t *opendcp, filelist_t *filelist);
int write_j2k_s_mxf(opendcp_t *opendcp, filelist_t *filelist);
int write_pcm_mxf(opendcp_t *opendcp, filelist_t *filelist);
int write_tt_mxf(opendcp_t *opendcp, filelist_t *filelist);
int write_mpeg2_mxf(opendcp_t *opendcp, filelist_t *filelist);

int j2k_context_create(opendcp_t *opendcp, filelist_t *filelist, char *output_file);
int j2k_s_context_create(opendcp_t *opendcp, filelist_t *filelist, char *output_file);
int pcm_context_create(opendcp_t *opendcp, filelist_t *filelist, char *output_file);

/* generate a random UUID */
extern "C" void uuid_random(char *uuid) {
    char buffer[64];
    Kumu::UUID TmpID;
    Kumu::GenRandomValue(TmpID);
    sprintf(uuid, "%.36s", TmpID.EncodeHex(buffer, 64));
}

/* calcuate the SHA1 digest of a file */
extern "C" int calculate_digest(opendcp_t *opendcp, const char *filename, char *digest) {
    using namespace Kumu;

    FileReader    reader;
    sha1_t        sha_context;
    ByteString    read_buffer(FILE_READ_SIZE);
    ui32_t        read_length;
    const ui32_t  sha_length = 20;
    byte_t        byte_buffer[sha_length];
    char          sha_buffer[64];
    Result_t      result = RESULT_OK;

    result = reader.OpenRead(filename);
    sha1_init(&sha_context);

    while (ASDCP_SUCCESS(result)) {
        read_length = 0;
        result = reader.Read(read_buffer.Data(), read_buffer.Capacity(), &read_length);

        if (ASDCP_SUCCESS(result)) {
            sha1_update(&sha_context, read_buffer.Data(), read_length);

            /* update callback (also check for interrupt) */
            if (opendcp->dcp.sha1_update.callback(opendcp->dcp.sha1_update.argument)) {
                return OPENDCP_CALC_DIGEST;
            }
        }
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_SUCCESS(result)) {
        sha1_final(byte_buffer, &sha_context);
        sprintf(digest, "%.36s", base64encode(byte_buffer, sha_length, sha_buffer, 64));
    }

    if (opendcp->dcp.sha1_done.callback(opendcp->dcp.sha1_done.argument)) {
        return OPENDCP_CALC_DIGEST;
    }

    if (ASDCP_SUCCESS(result)) {
        return OPENDCP_NO_ERROR;
    } else {
        return OPENDCP_CALC_DIGEST;
    }
}

/* get wav duration */
extern "C" int get_wav_duration(const char *filename, int frame_rate) {
    PCM::WAVParser       pcm_parser;
    PCM::AudioDescriptor audio_desc;
    Result_t             result   = RESULT_OK;
    ui32_t               duration = 0;

    Rational edit_rate(frame_rate, 1);

    result = pcm_parser.OpenRead(filename, edit_rate);

    if (ASDCP_SUCCESS(result)) {
        pcm_parser.FillAudioDescriptor(audio_desc);
        duration = audio_desc.ContainerDuration;
    }

    return duration;
}

/* get wav file info  */
extern "C" int get_wav_info(const char *filename, int frame_rate, wav_info_t *wav) {
    PCM::WAVParser       pcm_parser;
    PCM::AudioDescriptor audio_desc;
    Result_t             result   = RESULT_OK;

    Rational edit_rate(frame_rate, 1);

    result = pcm_parser.OpenRead(filename, edit_rate);

    if (ASDCP_SUCCESS(result)) {
        pcm_parser.FillAudioDescriptor(audio_desc);
        wav->nframes    = audio_desc.ContainerDuration;
        wav->nchannels  = audio_desc.ChannelCount;
        wav->bitdepth   = audio_desc.QuantizationBits;
        wav->samplerate = audio_desc.AudioSamplingRate.Numerator;
    } else {
        return OPENDCP_FILEOPEN_WAV;
    }

    return OPENDCP_NO_ERROR;
}

/* get the essence class of a file */
extern "C" int get_file_essence_class(char *filename, int raw) {
    Result_t      result = RESULT_OK;
    EssenceType_t essence_type;

    OPENDCP_LOG(LOG_DEBUG, "Reading file EssenceType: %s", filename);

    if (raw) {
        result = ASDCP::RawEssenceType(filename, essence_type);
    } else {
        result = ASDCP::EssenceType(filename, essence_type);
    }

    /* If type is unknown, return */
    if (ASDCP_FAILURE(result) || essence_type == ESS_UNKNOWN) {
        OPENDCP_LOG(LOG_DEBUG, "Unable to determine essence type");
        return ACT_UNKNOWN;
    }

    switch (essence_type) {
        case ESS_JPEG_2000:
        case ESS_JPEG_2000_S:
        case ESS_MPEG2_VES:
            return  ACT_PICTURE;
            break;

        case ESS_PCM_24b_48k:
        case ESS_PCM_24b_96k:
            return ACT_SOUND;
            break;

        case ESS_TIMED_TEXT:
            return ACT_TIMED_TEXT;
            break;

        default:
            return ACT_UNKNOWN;
            break;
    }

    return ACT_UNKNOWN;
}

/* get the essence type of an asset */
extern "C" int get_file_essence_type(char *filename) {
    Result_t result = RESULT_OK;
    EssenceType_t essence_type;

    result = ASDCP::RawEssenceType(filename, essence_type);

    if (ASDCP_FAILURE(result)) {
        return AET_UNKNOWN;
    }

    switch(essence_type) {
        case ESS_MPEG2_VES:
            return AET_MPEG2_VES;
            break;

        case ESS_JPEG_2000:
            return AET_JPEG_2000;
            break;

        case ESS_PCM_24b_48k:
            return AET_PCM_24b_48k;
            break;

        case ESS_PCM_24b_96k:
            return AET_PCM_24b_96k;
            break;

        case ESS_TIMED_TEXT:
            return AET_TIMED_TEXT;
            break;

        default:
            return AET_UNKNOWN;
            break;
    }

    return AET_UNKNOWN;
}

/* read asset file information */
extern "C" int read_asset_info(asset_t *asset) {
    EssenceType_t essence_type;
    WriterInfo info;
    Result_t result = RESULT_OK;
    Kumu::UUID uuid;
    char uuid_buffer[64];

    result = ASDCP::EssenceType(asset->filename, essence_type);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_DETECT_TRACK_TYPE;
    }

    switch (essence_type) {
        case ESS_MPEG2_VES:
        {
            MPEG2::MXFReader reader;
            MPEG2::VideoDescriptor desc;
            result = reader.OpenRead(asset->filename);

            if (ASDCP_FAILURE(result)) {
                return OPENDCP_FILEOPEN_MPEG2;
            }

            reader.FillVideoDescriptor(desc);
            reader.FillWriterInfo(info);

            asset->essence_type       = essence_type;
            asset->essence_class      = ACT_PICTURE;
            asset->duration           = desc.ContainerDuration;
            asset->intrinsic_duration = desc.ContainerDuration;
            asset->entry_point        = 0;
            asset->xml_ns             = info.LabelSetType;
            sprintf(asset->uuid, "%.36s", Kumu::bin2UUIDhex(info.AssetUUID, 16, uuid_buffer, 64));
            sprintf(asset->aspect_ratio, "%d %d", desc.AspectRatio.Numerator, desc.AspectRatio.Denominator);
            sprintf(asset->edit_rate, "%d %d", desc.EditRate.Numerator, desc.EditRate.Denominator);
            sprintf(asset->sample_rate, "%d %d", desc.SampleRate.Numerator, desc.SampleRate.Denominator);
            sprintf(asset->frame_rate, "%d", desc.FrameRate);
            break;
        }

        case ESS_JPEG_2000_S:
        {
            JP2K::MXFSReader reader;
            JP2K::PictureDescriptor desc;
            result = reader.OpenRead(asset->filename);

            if (ASDCP_FAILURE(result)) {
                return OPENDCP_FILEOPEN_J2K;
            }

            reader.FillPictureDescriptor(desc);
            reader.FillWriterInfo(info);

            asset->stereoscopic       = 1;
            asset->essence_type       = essence_type;
            asset->essence_class      = ACT_PICTURE;
            asset->duration           = desc.ContainerDuration;
            asset->intrinsic_duration = desc.ContainerDuration;
            asset->entry_point        = 0;
            asset->xml_ns             = info.LabelSetType;
            sprintf(asset->uuid, "%.36s", Kumu::bin2UUIDhex(info.AssetUUID, 16, uuid_buffer, 64));
            sprintf(asset->aspect_ratio, "%d %d", desc.AspectRatio.Numerator, desc.AspectRatio.Denominator);
            sprintf(asset->edit_rate, "%d %d", desc.EditRate.Numerator, desc.EditRate.Denominator);
            sprintf(asset->sample_rate, "%d %d", desc.SampleRate.Numerator, desc.SampleRate.Denominator);
            sprintf(asset->frame_rate, "%d %d", desc.SampleRate.Numerator, desc.SampleRate.Denominator);
            break;
        }

        case ESS_JPEG_2000:
        {
            JP2K::MXFReader reader;
            JP2K::PictureDescriptor desc;
            result = reader.OpenRead(asset->filename);

            /* Try 3D MXF Interop */
            if (ASDCP_FAILURE(result)) {
                JP2K::MXFSReader reader;
                result = reader.OpenRead(asset->filename);
                asset->stereoscopic   = 1;

                if ( ASDCP_FAILURE(result) ) {
                    return OPENDCP_FILEOPEN_J2K;
                }

                reader.FillPictureDescriptor(desc);
                reader.FillWriterInfo(info);
            } else {
                reader.FillPictureDescriptor(desc);
                reader.FillWriterInfo(info);
            }

            asset->essence_type        = essence_type;
            asset->essence_class       = ACT_PICTURE;
            asset->duration            = desc.ContainerDuration;
            asset->intrinsic_duration  = desc.ContainerDuration;
            asset->entry_point    = 0;
            asset->xml_ns         = info.LabelSetType;
            sprintf(asset->uuid, "%.36s", Kumu::bin2UUIDhex(info.AssetUUID, 16, uuid_buffer, 64));
            sprintf(asset->aspect_ratio, "%d %d", desc.AspectRatio.Numerator, desc.AspectRatio.Denominator);
            sprintf(asset->edit_rate, "%d %d", desc.EditRate.Numerator, desc.EditRate.Denominator);
            sprintf(asset->sample_rate, "%d %d", desc.SampleRate.Numerator, desc.SampleRate.Denominator);
            sprintf(asset->frame_rate, "%d %d", desc.SampleRate.Numerator, desc.SampleRate.Denominator);
            break;
        }

        case ESS_PCM_24b_48k:
        case ESS_PCM_24b_96k:
        {
            PCM::MXFReader reader;
            PCM::AudioDescriptor desc;
            result = reader.OpenRead(asset->filename);

            if (ASDCP_FAILURE(result)) {
                return OPENDCP_FILEOPEN_WAV;
            }

            reader.FillAudioDescriptor(desc);
            reader.FillWriterInfo(info);

            asset->essence_type       = essence_type;
            asset->essence_class      = ACT_SOUND;
            asset->duration           = desc.ContainerDuration;
            asset->intrinsic_duration = desc.ContainerDuration;
            asset->entry_point    = 0;
            asset->xml_ns         = info.LabelSetType;
            sprintf(asset->uuid, "%.36s", Kumu::bin2UUIDhex(info.AssetUUID, 16, uuid_buffer, 64));
            sprintf(asset->edit_rate, "%d %d", desc.EditRate.Numerator, desc.EditRate.Denominator);
            sprintf(asset->sample_rate, "%d %d", desc.AudioSamplingRate.Numerator, desc.AudioSamplingRate.Denominator);
            break;
        }

        case ESS_TIMED_TEXT:
        {
            TimedText::MXFReader reader;
            TimedText::TimedTextDescriptor desc;
            result = reader.OpenRead(asset->filename);

            if (ASDCP_FAILURE(result)) {
                return OPENDCP_FILEOPEN_TT;
            }

            reader.FillTimedTextDescriptor(desc);
            reader.FillWriterInfo(info);

            asset->essence_type       = essence_type;
            asset->essence_class      = ACT_TIMED_TEXT;
            asset->duration           = desc.ContainerDuration;
            asset->intrinsic_duration = desc.ContainerDuration;
            asset->entry_point    = 0;
            asset->xml_ns         = info.LabelSetType;
            sprintf(asset->uuid, "%.36s", Kumu::bin2UUIDhex(info.AssetUUID, 16, uuid_buffer, 64));
            sprintf(asset->edit_rate, "%d %d", desc.EditRate.Numerator, desc.EditRate.Denominator);
            break;
        }

        default:
        {
            return OPENDCP_UNKNOWN_TRACK_TYPE;
            break;
        }
    }

    /* add encrypted info, if applicable */
    if (info.EncryptedEssence) {
        asset->encrypted  = 1;
        sprintf(asset->key_id, "%.36s", Kumu::bin2UUIDhex(info.CryptographicKeyID, 16, uuid_buffer, 64));
    } else {
        asset->encrypted  = 0;
    }

    return OPENDCP_NO_ERROR;
}

extern "C" int mxf_create(opendcp_t *opendcp, filelist_t *filelist, char *output_file) {
    Result_t      result = RESULT_OK;
    EssenceType_t essence_type;

    result = ASDCP::RawEssenceType(filelist->files[0], essence_type);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_DETECT_TRACK_TYPE;
    }

    opendcp->mxf.type = essence_type;

    /* set the starting frame */
    if (opendcp->mxf.start_frame && filelist->nfiles >= (opendcp->mxf.start_frame - 1)) {
        opendcp->mxf.start_frame = opendcp->mxf.start_frame - 1;  /* adjust for zero base */
    } else {
        opendcp->mxf.start_frame = 0;
    }

    OPENDCP_LOG(LOG_INFO, "write_j2k_mxf: %x", opendcp->mxf.asdcp);
    OPENDCP_LOG(LOG_INFO, "Returning MXF Context");

    switch (opendcp->mxf.type) {
        case ESS_JPEG_2000:
            if (opendcp->stereoscopic) {
                OPENDCP_LOG(LOG_INFO, "Creating J2K 3D MXF Context");
                if (j2k_s_context_create(opendcp, filelist, output_file) != OPENDCP_NO_ERROR) {
                    return OPENDCP_ERROR;
                }
            } else {
                OPENDCP_LOG(LOG_INFO, "Creating J2K MXF Context");
                if (j2k_context_create(opendcp, filelist, output_file) != OPENDCP_NO_ERROR) {
                    return OPENDCP_ERROR;
                }
            }
            break;

        case ESS_JPEG_2000_S:
            OPENDCP_LOG(LOG_INFO, "Creating J2K 3D MXF Context");
            if (j2k_s_context_create(opendcp, filelist, output_file) != OPENDCP_NO_ERROR) {
                return OPENDCP_ERROR;
            }
            break;

        case ESS_PCM_24b_48k:
        case ESS_PCM_24b_96k:
            OPENDCP_LOG(LOG_INFO, "Creating J2K 3D MXF Context");
            if (pcm_context_create(opendcp, filelist, output_file) != OPENDCP_NO_ERROR) {
                return OPENDCP_ERROR;
            }
            break;

/*
        case ESS_MPEG2_VES:
            return write_mpeg2_mxf(opendcp, filelist, output_file);
            break;

        case ESS_TIMED_TEXT:
            return write_tt_mxf(opendcp, filelist, output_file);
            break;

        case ESS_UNKNOWN:
            return OPENDCP_UNKNOWN_TRACK_TYPE;
            break;
*/
        default:
            return OPENDCP_UNKNOWN_TRACK_TYPE;
            break;
    }

    OPENDCP_LOG(LOG_INFO, "write_j2k_mxf: %x", opendcp->mxf.asdcp);
    OPENDCP_LOG(LOG_INFO, "Returning MXF Context");

    return OPENDCP_NO_ERROR;
}

/* write the asset to an mxf file */
extern "C" int write_mxf(opendcp_t *opendcp, char *mxf_ptr, filelist_t *filelist) {
    Result_t      result = RESULT_OK;

    switch (opendcp->mxf.type) {
        case ESS_JPEG_2000:
            if (opendcp->stereoscopic) {
                return write_j2k_s_mxf(opendcp, filelist);
            } else {
                return write_j2k_mxf(opendcp, filelist);
            }
            break;

        case ESS_JPEG_2000_S:
            return write_j2k_s_mxf(opendcp, filelist);
            break;

        case ESS_PCM_24b_48k:
        case ESS_PCM_24b_96k:
            return write_pcm_mxf(opendcp, filelist);
            break;

        case ESS_MPEG2_VES:
            return write_mpeg2_mxf(opendcp, filelist);
            break;

        case ESS_TIMED_TEXT:
            return write_tt_mxf(opendcp, filelist);
            break;

        case ESS_UNKNOWN:
            return OPENDCP_UNKNOWN_TRACK_TYPE;
            break;

        default:
            return OPENDCP_UNKNOWN_TRACK_TYPE;
            break;
    }

    return OPENDCP_UNKNOWN_TRACK_TYPE;
}

Result_t fill_writer_info(opendcp_t *opendcp, writer_info_t *writer_info) {
    Kumu::FortunaRNG        rng;
    byte_t                  iv_buf[CBC_BLOCK_SIZE];
    Result_t                result = RESULT_OK;

    writer_info->info.ProductVersion = OPENDCP_VERSION;
    writer_info->info.CompanyName = OPENDCP_NAME;
    writer_info->info.ProductName = OPENDCP_NAME;

    /* set the label type */
    if (opendcp->ns == XML_NS_INTEROP) {
        writer_info->info.LabelSetType = LS_MXF_INTEROP;
    } else if (opendcp->ns == XML_NS_SMPTE) {
        writer_info->info.LabelSetType = LS_MXF_SMPTE;
    } else {
        writer_info->info.LabelSetType = LS_MXF_UNKNOWN;
    }

    /* generate a random UUID for this essence */
    Kumu::GenRandomUUID(writer_info->info.AssetUUID);

    /* start encryption, if set */
    if (opendcp->mxf.key_flag) {
        Kumu::GenRandomUUID(writer_info->info.ContextID);
        writer_info->info.EncryptedEssence = true;

        if (is_key_value_set(opendcp->mxf.key_id, sizeof(opendcp->mxf.key_id))) {
            memcpy(writer_info->info.CryptographicKeyID, opendcp->mxf.key_id, UUIDlen);
        } else {
            rng.FillRandom(writer_info->info.CryptographicKeyID, UUIDlen);
        }

        writer_info->aes_context = new AESEncContext;
        result = writer_info->aes_context->InitKey(opendcp->mxf.key_value);

        if (ASDCP_FAILURE(result)) {
            return result;
        }

        result = writer_info->aes_context->SetIVec(rng.FillRandom(iv_buf, CBC_BLOCK_SIZE));

        if (ASDCP_FAILURE(result)) {
            return result;
        }

        if (opendcp->mxf.write_hmac) {
            writer_info->info.UsesHMAC = true;
            writer_info->hmac_context = new HMACContext;
            result = writer_info->hmac_context->InitKey(opendcp->mxf.key_value, writer_info->info.LabelSetType);

            if (ASDCP_FAILURE(result)) {
                return result;
            }
        }
    }

    return result;
}

int j2k_context_create(opendcp_t *opendcp, filelist_t *filelist, char *output_file) {
    Result_t                result = RESULT_OK;
    JP2K::FrameBuffer       frame_buffer(FRAME_BUFFER_SIZE);

    j2k_context_t *j2k = new j2k_context_t;

    fill_writer_info(opendcp, &j2k->writer_info);

    result = j2k->parser.OpenReadFrame(filelist->files[opendcp->mxf.start_frame], frame_buffer);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_ERROR;
    }

    Rational edit_rate(opendcp->mxf.edit_rate, 1);
    j2k->parser.FillPictureDescriptor(j2k->picture_desc);
    j2k->picture_desc.EditRate = edit_rate;

    result = j2k->mxf_writer.OpenWrite(output_file, j2k->writer_info.info, j2k->picture_desc);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEWRITE_MXF;
    }

    opendcp->mxf.asdcp = (char *)j2k;

    return OPENDCP_NO_ERROR;
}

int j2k_s_context_create(opendcp_t *opendcp, filelist_t *filelist, char *output_file) {
    Result_t                result = RESULT_OK;
    JP2K::FrameBuffer       frame_buffer(FRAME_BUFFER_SIZE);

    j2k_s_context_t *j2k = new j2k_s_context_t;

    fill_writer_info(opendcp, &j2k->writer_info);

    result = j2k->parser.OpenReadFrame(filelist->files[0], frame_buffer);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEOPEN_J2K;
    }

    Rational edit_rate(opendcp->mxf.edit_rate, 1);
    j2k->parser.FillPictureDescriptor(j2k->picture_desc);
    j2k->picture_desc.EditRate = edit_rate;

    result = j2k->mxf_writer.OpenWrite(output_file, j2k->writer_info.info, j2k->picture_desc);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEWRITE_MXF;
    }

    opendcp->mxf.asdcp = (char *)j2k;

    return OPENDCP_NO_ERROR;
}

int write_mxf_frame(opendcp_t *opendcp, char *frame) {
    UNUSED(frame);
    UNUSED(opendcp);
    return 0;
}

/* write out j2k mxf file */
int write_j2k_mxf(opendcp_t *opendcp, filelist_t *filelist) {
    Result_t               result = RESULT_OK;
    JP2K::FrameBuffer      frame_buffer(FRAME_BUFFER_SIZE);
    int                    i, j;

    j2k_context_t *j2k = (j2k_context_t *)opendcp->mxf.asdcp;

    /* read each input frame and write to the output mxf until duration is reached */
    for (i = opendcp->mxf.start_frame; i < opendcp->mxf.end_frame && ASDCP_SUCCESS(result); i++) {
        result = j2k->parser.OpenReadFrame(filelist->files[i], frame_buffer);

        if (opendcp->mxf.delete_intermediate) {
            unlink(filelist->files[i]);
        }

        if (ASDCP_FAILURE(result)) {
            return OPENDCP_FILEOPEN_J2K;
        }

        if (opendcp->mxf.encrypt_header_flag) {
            frame_buffer.PlaintextOffset(0);
        }

        /* write frame(s) */
        for (j = 0; j < opendcp->mxf.frame_duration; j++) {
            result = j2k->mxf_writer.WriteFrame(frame_buffer, j2k->writer_info.aes_context, j2k->writer_info.hmac_context);

            /* frame done callback (also check for interrupt) */
            if (opendcp->mxf.frame_done.callback(opendcp->mxf.frame_done.argument)) {
                return OPENDCP_NO_ERROR;
            }
        }
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEWRITE_MXF;
    }

    result = j2k->mxf_writer.Finalize();

    /* file done callback */
    opendcp->mxf.file_done.callback(opendcp->mxf.file_done.argument);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FINALIZE_MXF;
    }

    return OPENDCP_NO_ERROR;
}

/* write out 3D j2k mxf file */
int write_j2k_s_mxf(opendcp_t *opendcp, filelist_t *filelist) {
    Result_t                    result = RESULT_OK;
    JP2K::FrameBuffer           frame_buffer_left(FRAME_BUFFER_SIZE);
    JP2K::FrameBuffer           frame_buffer_right(FRAME_BUFFER_SIZE);
    int                         i, j;

    j2k_s_context_t *j2k = (j2k_s_context_t *)opendcp->mxf.asdcp;

    /* read each input frame and write to the output mxf until duration is reached */
    for (i = opendcp->mxf.start_frame; i < opendcp->mxf.end_frame && ASDCP_SUCCESS(result); i+=2) {
        result = j2k->parser.OpenReadFrame(filelist->files[i], frame_buffer_left);

        if (ASDCP_FAILURE(result)) {
            return OPENDCP_FILEOPEN_J2K;
        }

        result = j2k->parser.OpenReadFrame(filelist->files[i+1], frame_buffer_right);

        if (ASDCP_FAILURE(result)) {
            return OPENDCP_FILEOPEN_J2K;
        }

        if (opendcp->mxf.encrypt_header_flag) {
            frame_buffer_left.PlaintextOffset(0);
            frame_buffer_right.PlaintextOffset(0);
        }

        /* write frame(s) */
        for (j = 0; j < opendcp->mxf.frame_duration; j++) {
            result = j2k->mxf_writer.WriteFrame(frame_buffer_left, JP2K::SP_LEFT, j2k->writer_info.aes_context, j2k->writer_info.hmac_context);
            result = j2k->mxf_writer.WriteFrame(frame_buffer_right, JP2K::SP_RIGHT, j2k->writer_info.aes_context, j2k->writer_info.hmac_context);

            /* frame done callback (also check for interrupt) */
            opendcp->mxf.frame_done.callback(opendcp->mxf.frame_done.argument);
        }
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEWRITE_MXF;
    }

    result = j2k->mxf_writer.Finalize();

    opendcp->mxf.file_done.callback(opendcp->mxf.file_done.argument);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FINALIZE_MXF;
    }

    return OPENDCP_NO_ERROR;
}

int pcm_context_create(opendcp_t *opendcp, filelist_t *filelist, char *output_file) {
    Result_t                result = RESULT_OK;
    PCM::FrameBuffer        frame_buffer(FRAME_BUFFER_SIZE);
    PCM::WAVParser          parser;
    PCM::AudioDescriptor    audio_desc;
    int                     i;

    pcm_context_t *pcm = new pcm_context_t;

    fill_writer_info(opendcp, &pcm->writer_info);

    Rational edit_rate(opendcp->mxf.edit_rate, 1);

    /* read first file */
    result = pcm->parser.OpenRead(filelist->files[0], edit_rate);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEOPEN_WAV;
    }

    /* read audio descriptor */
    pcm->parser.FillAudioDescriptor(pcm->audio_desc);
    pcm->audio_desc.ChannelCount = 0;
    pcm->audio_desc.BlockAlign   = 0;

    for (i = 0; i < filelist->nfiles; i++) {
        result = pcm->parser_channel[i].OpenRead(filelist->files[i], edit_rate);

        if (ASDCP_FAILURE(result)) {
            OPENDCP_LOG(LOG_ERROR, "could not open %s", filelist->files[i]);
            return OPENDCP_FILEOPEN_WAV;
        }

        pcm->parser_channel[i].FillAudioDescriptor(audio_desc);

        if (audio_desc.AudioSamplingRate != pcm->audio_desc.AudioSamplingRate) {
            OPENDCP_LOG(LOG_ERROR, "mismatched sampling rate");
            return OPENDCP_FILEOPEN_WAV;
        }

        if (audio_desc.QuantizationBits != pcm->audio_desc.QuantizationBits) {
            OPENDCP_LOG(LOG_ERROR, "mismatched bit rate");
            return OPENDCP_FILEOPEN_WAV;
        }

        if (audio_desc.ContainerDuration != pcm->audio_desc.ContainerDuration) {
            OPENDCP_LOG(LOG_ERROR, "mismatched duration");
            return OPENDCP_FILEOPEN_WAV;
        }

        pcm->audio_desc.ChannelCount += audio_desc.ChannelCount;
        pcm->audio_desc_channel[i] = audio_desc;
    }

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEOPEN_WAV;
    }

    /* DCP should be at least MIN channels, we'll create black wavs later if missing */
    if (pcm->audio_desc.ChannelCount < MIN_AUDIO_CHANNELS) {
        OPENDCP_LOG(LOG_INFO, "audio channels (%d) less than minimum (%d), creating black wavs", pcm->audio_desc.ChannelCount, MIN_AUDIO_CHANNELS);
        pcm->audio_desc.ChannelCount = MIN_AUDIO_CHANNELS;
    }

    /*  set total audio characteristics */
    pcm->audio_desc.BlockAlign = pcm->audio_desc.ChannelCount * 24 / 8;
    pcm->audio_desc.EditRate = edit_rate;
    pcm->audio_desc.AvgBps   = pcm->audio_desc.AvgBps * filelist->nfiles;

    result = pcm->mxf_writer.OpenWrite(output_file, pcm->writer_info.info, pcm->audio_desc);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEWRITE_MXF;
    }

    opendcp->mxf.asdcp = (char *)pcm;

    return OPENDCP_NO_ERROR;
}

/* write out pcm audio mxf file */
int write_pcm_mxf(opendcp_t *opendcp, filelist_t *filelist) {
    Result_t             result = RESULT_OK;
    PCM::FrameBuffer     frame_buffer;
    PCM::FrameBuffer     frame_buffer_channel[MAX_AUDIO_CHANNELS];
    int                  i, j;
    int                  channels_read;

    pcm_context_t *pcm = (pcm_context_t *)opendcp->mxf.asdcp;

    /* set total output frame buffer size */
    frame_buffer.Capacity(PCM::CalcFrameBufferSize(pcm->audio_desc));
    frame_buffer.Size(PCM::CalcFrameBufferSize(pcm->audio_desc));

    for (i = opendcp->mxf.start_frame; i < opendcp->mxf.end_frame && ASDCP_SUCCESS(result); i++) {
        byte_t *data_s = frame_buffer.Data();
        byte_t *data_e = data_s + frame_buffer.Capacity();
        byte_t sample_size = PCM::CalcSampleSize(pcm->audio_desc_channel[0]);
        int    offset = 0;

        /* read a frame from each file */
        channels_read = 0;
        for (j = 0; j < filelist->nfiles; j++) {
            channels_read += pcm->audio_desc_channel[j].ChannelCount;
            frame_buffer_channel[j].Capacity(PCM::CalcFrameBufferSize(pcm->audio_desc_channel[j]));
            memset(frame_buffer_channel[j].Data(), 0, frame_buffer_channel[j].Capacity());
            result = pcm->parser_channel[j].ReadFrame(frame_buffer_channel[j]);

            if (ASDCP_FAILURE(result)) {
                continue;
            }

            if (frame_buffer_channel[j].Size() != frame_buffer_channel[j].Capacity()) {
                OPENDCP_LOG(LOG_INFO, "frame size mismatch, expect size: %d did match actual size: %d",
                            frame_buffer_channel[j].Capacity(), frame_buffer_channel[j].Size());
                result = RESULT_ENDOFFILE;
                continue;
            }
        }

        for (j = channels_read; j < 6; j++) {
            channels_read += 1;
            frame_buffer_channel[j].Capacity(PCM::CalcFrameBufferSize(pcm->audio_desc_channel[0]));
            memset(frame_buffer_channel[j].Data(), 0, frame_buffer_channel[j].Capacity());
        }

        /* write samples from each frame to output buffer */
        if (ASDCP_SUCCESS(result)) {
            while (data_s < data_e) {
                for (j = 0; j < channels_read; j++) {
                    byte_t *frame = frame_buffer_channel[j].Data() + offset;
                    memcpy(data_s, frame, sample_size);
                    data_s += sample_size;
                }

                offset += sample_size;
            }

            /* write the frame */
            result = pcm->mxf_writer.WriteFrame(frame_buffer, pcm->writer_info.aes_context, pcm->writer_info.hmac_context);

            /* frame done callback (also check for interrupt) */
            opendcp->mxf.frame_done.callback(opendcp->mxf.frame_done.argument);
        }
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEWRITE_MXF;
    }

    /* write footer information */
    result = pcm->mxf_writer.Finalize();

    opendcp->mxf.file_done.callback(opendcp->mxf.file_done.argument);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FINALIZE_MXF;
    }

    return OPENDCP_NO_ERROR;
}

/* write out timed text mxf file */
int write_tt_mxf(opendcp_t *opendcp, filelist_t *filelist) {
    TimedText::DCSubtitleParser    tt_parser;
    TimedText::MXFWriter           mxf_writer;
    TimedText::FrameBuffer         frame_buffer(FRAME_BUFFER_SIZE);
    TimedText::TimedTextDescriptor tt_desc;
    TimedText::ResourceList_t::const_iterator resource_iterator;
    writer_info_t                  writer_info;
    std::string                    xml_doc;
    Result_t                       result = RESULT_OK;

    char *output_file = NULL;

    result = tt_parser.OpenRead(filelist->files[0]);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEOPEN_TT;
    }

    tt_parser.FillTimedTextDescriptor(tt_desc);

    fill_writer_info(opendcp, &writer_info);

    result = mxf_writer.OpenWrite(output_file, writer_info.info, tt_desc);
    result = tt_parser.ReadTimedTextResource(xml_doc);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEOPEN_TT;
    }

    result = mxf_writer.WriteTimedTextResource(xml_doc, writer_info.aes_context, writer_info.hmac_context);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEWRITE_MXF;
    }

    resource_iterator = tt_desc.ResourceList.begin();

    while (ASDCP_SUCCESS(result) && resource_iterator != tt_desc.ResourceList.end()) {
        result = tt_parser.ReadAncillaryResource((*resource_iterator++).ResourceID, frame_buffer);

        if (ASDCP_FAILURE(result)) {
            return OPENDCP_FILEOPEN_TT;
        }

        result = mxf_writer.WriteAncillaryResource(frame_buffer, writer_info.aes_context, writer_info.hmac_context);
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEWRITE_MXF;
    }

    result = mxf_writer.Finalize();

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FINALIZE_MXF;
    }

    return OPENDCP_NO_ERROR;
}

/* write out mpeg2 mxf file */
int write_mpeg2_mxf(opendcp_t *opendcp, filelist_t *filelist) {
    MPEG2::FrameBuffer     frame_buffer(FRAME_BUFFER_SIZE);
    MPEG2::Parser          mpeg2_parser;
    MPEG2::MXFWriter       mxf_writer;
    MPEG2::VideoDescriptor video_desc;
    writer_info_t          writer_info;
    Result_t               result = RESULT_OK;
    ui32_t                 mxf_duration;

char *output_file = NULL;
    result = mpeg2_parser.OpenRead(filelist->files[0]);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEOPEN_MPEG2;
    }

    mpeg2_parser.FillVideoDescriptor(video_desc);

    fill_writer_info(opendcp, &writer_info);

    result = mxf_writer.OpenWrite(output_file, writer_info.info, video_desc);

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEWRITE_MXF;
    }

    result = mpeg2_parser.Reset();

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_PARSER_RESET;
    }

    if (!opendcp->duration) {
        mxf_duration = 0xffffffff;
    } else {
        mxf_duration = opendcp->duration;
    }

    while (ASDCP_SUCCESS(result) && mxf_duration--) {
        result = mpeg2_parser.ReadFrame(frame_buffer);

        if (ASDCP_FAILURE(result)) {
            continue;
        }

        if (opendcp->mxf.encrypt_header_flag) {
            frame_buffer.PlaintextOffset(0);
        }

        result = mxf_writer.WriteFrame(frame_buffer, writer_info.aes_context, writer_info.hmac_context);
    }

    if (result == RESULT_ENDOFFILE) {
        result = RESULT_OK;
    }

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FILEWRITE_MXF;
    }

    result = mxf_writer.Finalize();

    if (ASDCP_FAILURE(result)) {
        return OPENDCP_FINALIZE_MXF;
    }

    return OPENDCP_NO_ERROR;
}
