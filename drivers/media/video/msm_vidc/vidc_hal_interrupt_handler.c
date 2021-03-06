/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/slab.h>
#include <linux/list.h>
#include "vidc_hal.h"

static enum vidc_status vidc_map_hal_err_status(enum HFI_ERROR hfi_err)
{
	enum vidc_status vidc_err;
	switch (hfi_err) {
	case HFI_ERR_NONE:
	case HFI_ERR_SESSION_SAME_STATE_OPERATION:
		vidc_err = VIDC_ERR_NONE;
		break;
	case HFI_ERR_SYS_FATAL:
		vidc_err = VIDC_ERR_HW_FATAL;
		break;
	case HFI_ERR_SYS_VERSION_MISMATCH:
	case HFI_ERR_SYS_INVALID_PARAMETER:
	case HFI_ERR_SYS_SESSION_ID_OUT_OF_RANGE:
	case HFI_ERR_SESSION_INVALID_PARAMETER:
	case HFI_ERR_SESSION_INVALID_SESSION_ID:
	case HFI_ERR_SESSION_INVALID_STREAM_ID:
		vidc_err = VIDC_ERR_BAD_PARAM;
		break;
	case HFI_ERR_SYS_INSUFFICIENT_RESOURCES:
	case HFI_ERR_SYS_UNSUPPORTED_DOMAIN:
	case HFI_ERR_SYS_UNSUPPORTED_CODEC:
	case HFI_ERR_SESSION_UNSUPPORTED_PROPERTY:
	case HFI_ERR_SESSION_UNSUPPORTED_SETTING:
	case HFI_ERR_SESSION_INSUFFICIENT_RESOURCES:
		vidc_err = VIDC_ERR_NOT_SUPPORTED;
		break;
	case HFI_ERR_SYS_MAX_SESSIONS_REACHED:
		vidc_err = VIDC_ERR_MAX_CLIENT;
		break;
	case HFI_ERR_SYS_SESSION_IN_USE:
		vidc_err = VIDC_ERR_CLIENT_PRESENT;
		break;
	case HFI_ERR_SESSION_FATAL:
		vidc_err = VIDC_ERR_CLIENT_FATAL;
		break;
	case HFI_ERR_SESSION_BAD_POINTER:
		vidc_err = VIDC_ERR_BAD_PARAM;
		break;
	case HFI_ERR_SESSION_INCORRECT_STATE_OPERATION:
		vidc_err = VIDC_ERR_BAD_STATE;
		break;
	case HFI_ERR_SESSION_STREAM_CORRUPT:
	case HFI_ERR_SESSION_STREAM_CORRUPT_OUTPUT_STALLED:
		vidc_err = VIDC_ERR_BITSTREAM_ERR;
		break;
	case HFI_ERR_SESSION_SYNC_FRAME_NOT_DETECTED:
		vidc_err = VIDC_ERR_IFRAME_EXPECTED;
		break;
	case HFI_ERR_SYS_UNKNOWN:
	case HFI_ERR_SESSION_UNKNOWN:
	case HFI_ERR_SESSION_EMPTY_BUFFER_DONE_OUTPUT_PENDING:
	default:
		vidc_err = VIDC_ERR_FAIL;
		break;
	}
	return vidc_err;
}

void hal_process_sess_evt_seq_changed(struct hal_device *device,
	struct hfi_msg_event_notify_packet *pkt)
{
	struct msm_vidc_cb_cmd_done cmd_done;
	struct msm_vidc_cb_event event_notify;
	int num_properties_changed;
	struct hfi_frame_size frame_sz;
	u8 *data_ptr;
	enum HFI_PROPERTY prop_id;
	HAL_MSG_LOW("RECEIVED:EVENT_NOTIFY");
	if (sizeof(struct hfi_msg_event_notify_packet)
		> pkt->size) {
		HAL_MSG_ERROR("hal_process_session_init_done:bad_pkt_size");
		return;
	}

	memset(&cmd_done, 0, sizeof(struct msm_vidc_cb_cmd_done));
	memset(&event_notify, 0, sizeof(struct
				msm_vidc_cb_event));

	cmd_done.device_id = device->device_id;
	cmd_done.session_id = ((struct hal_session *) pkt->session_id)->
		session_id;
	cmd_done.status = VIDC_ERR_NONE;
	cmd_done.size = sizeof(struct msm_vidc_cb_event);
	num_properties_changed = pkt->event_data2;
	if (num_properties_changed) {
		data_ptr = (u8 *) &pkt->rg_ext_event_data[0];
		do {
			prop_id = (enum HFI_PROPERTY) *((u32 *)data_ptr);
			switch (prop_id) {
			case HFI_PROPERTY_PARAM_FRAME_SIZE:
				frame_sz.buffer =
					(enum HFI_BUFFER)
						*((((u32 *)data_ptr)+1));
				frame_sz.width =
					event_notify.width =
						*((((u32 *)data_ptr)+2));
				frame_sz.height =
					event_notify.height =
						*((((u32 *)data_ptr)+3));
				data_ptr += 4;
			break;
			default:
			break;
			}
			num_properties_changed--;
		} while (num_properties_changed > 0);
	}
	cmd_done.data = &event_notify;
	device->callback(VIDC_EVENT_CHANGE, &cmd_done);
}

static void hal_process_event_notify(struct hal_device *device,
	struct hfi_msg_event_notify_packet *pkt)
{
	HAL_MSG_LOW("RECVD:EVENT_NOTIFY");

	if (!device || !pkt ||
		pkt->size < sizeof(struct hfi_msg_event_notify_packet)) {
		HAL_MSG_ERROR("Invalid Params in %s", __func__);
		return;
	}

	switch (pkt->event_id) {
	case HFI_EVENT_SYS_ERROR:
		HAL_MSG_INFO("HFI_EVENT_SYS_ERROR");
		break;
	case HFI_EVENT_SESSION_ERROR:
		HAL_MSG_INFO("HFI_EVENT_SESSION_ERROR");
		break;
	case HFI_EVENT_SESSION_SEQUENCE_CHANGED:
		HAL_MSG_INFO("HFI_EVENT_SESSION_SEQUENCE_CHANGED");
		hal_process_sess_evt_seq_changed(device, pkt);
		break;
	case HFI_EVENT_SESSION_PROPERTY_CHANGED:
		HAL_MSG_INFO("HFI_EVENT_SESSION_PROPERTY_CHANGED");
		break;
	default:
		HAL_MSG_INFO("hal_process_event_notify:unkown_event_id");
		break;
	}
}

static void hal_process_sys_init_done(struct hal_device *device,
		struct hfi_msg_sys_init_done_packet *pkt)
{
	struct msm_vidc_cb_cmd_done cmd_done;
	struct vidc_hal_sys_init_done sys_init_done;
	u32 rem_bytes, bytes_read = 0, num_properties;
	u8 *data_ptr;
	enum HFI_PROPERTY prop_id;
	enum vidc_status status = VIDC_ERR_NONE;

	HAL_MSG_LOW("RECEIVED:SYS_INIT_DONE");
	if (sizeof(struct hfi_msg_sys_init_done_packet) > pkt->size) {
		HAL_MSG_ERROR("hal_process_sys_init_done:bad_pkt_size: %d",
				pkt->size);
		return;
	}

	status = vidc_map_hal_err_status((u32)pkt->error_type);

	if (!status) {
		if (pkt->num_properties == 0) {
			HAL_MSG_ERROR("hal_process_sys_init_done:"
						"no_properties");
			status = VIDC_ERR_FAIL;
			goto err_no_prop;
		}

		rem_bytes = pkt->size - sizeof(struct
			hfi_msg_sys_init_done_packet) + sizeof(u32);

		if (rem_bytes == 0) {
			HAL_MSG_ERROR("hal_process_sys_init_done:"
						"missing_prop_info");
			status = VIDC_ERR_FAIL;
			goto err_no_prop;
		}
		memset(&cmd_done, 0, sizeof(struct msm_vidc_cb_cmd_done));
		memset(&sys_init_done, 0, sizeof(struct
				vidc_hal_sys_init_done));

		data_ptr = (u8 *) &pkt->rg_property_data[0];
		num_properties = pkt->num_properties;

		while ((num_properties != 0) && (rem_bytes >= sizeof(u32))) {
			prop_id = (enum HFI_PROPERTY) *((u32 *)data_ptr);
			data_ptr = data_ptr + 4;

			switch (prop_id) {
			case HFI_PROPERTY_PARAM_CODEC_SUPPORTED:
			{
				struct hfi_codec_supported *prop =
					(struct hfi_codec_supported *) data_ptr;
				if (rem_bytes < sizeof(struct
						hfi_codec_supported)) {
					status = VIDC_ERR_BAD_PARAM;
					break;
				}
				sys_init_done.dec_codec_supported =
					prop->decoder_codec_supported;
				sys_init_done.enc_codec_supported =
					prop->encoder_codec_supported;
				break;
			}
			default:
				HAL_MSG_ERROR("hal_process_sys_init_done:"
							"bad_prop_id");
				status = VIDC_ERR_BAD_PARAM;
				break;
			}
			if (!status) {
				rem_bytes -= bytes_read;
				data_ptr += bytes_read;
				num_properties--;
			}
		}
	}
err_no_prop:
	cmd_done.device_id = device->device_id;
	cmd_done.session_id = 0;
	cmd_done.status = (u32) status;
	cmd_done.size = sizeof(struct vidc_hal_sys_init_done);
	cmd_done.data = (void *) &sys_init_done;
	device->callback(SYS_INIT_DONE, &cmd_done);
}

enum vidc_status vidc_hal_process_sess_init_done_prop_read(
	struct hfi_msg_sys_session_init_done_packet *pkt,
	struct msm_vidc_cb_cmd_done *cmddone)
{
	return VIDC_ERR_NONE;
}

static void hal_process_sess_get_prop_buf_req(
	struct hfi_msg_session_property_info_packet *prop,
	struct buffer_requirements *buffreq)
{
	struct hfi_buffer_requirements *hfi_buf_req;
	u32 req_bytes;
	enum vidc_status rc = VIDC_ERR_NONE;

	HAL_MSG_LOW("Entered %s", __func__);
	req_bytes = prop->size - sizeof(
	struct hfi_msg_session_property_info_packet);

	if (req_bytes == 0 || (req_bytes % sizeof(
		struct hfi_buffer_requirements))) {
		HAL_MSG_ERROR("hal_process_sess_get_prop_buf_req:bad_pkt_size:"
					" %d", req_bytes);
		return;
	}

	hfi_buf_req = (struct hfi_buffer_requirements *)
		&prop->rg_property_data[1];

	while (req_bytes != 0) {
		if ((hfi_buf_req->buffer_count_min > hfi_buf_req->
			buffer_count_actual)
			|| (hfi_buf_req->buffer_alignment == 0)
			|| (hfi_buf_req->buffer_size == 0)) {
			HAL_MSG_ERROR("hal_process_sess_get_prop_buf_req:"
						"bad_buf_req");
			rc = VIDC_ERR_FAIL;
		}
		HAL_MSG_LOW("got buffer requirements for: %d",
					hfi_buf_req->buffer);
		switch (hfi_buf_req->buffer) {
		case HFI_BUFFER_INPUT:
			memcpy(&buffreq->buffer[0], hfi_buf_req,
				sizeof(struct hfi_buffer_requirements));
			buffreq->buffer[0].buffer_type = HAL_BUFFER_INPUT;
			break;
		case HFI_BUFFER_OUTPUT:
			memcpy(&buffreq->buffer[1], hfi_buf_req,
			sizeof(struct hfi_buffer_requirements));
			buffreq->buffer[1].buffer_type = HAL_BUFFER_OUTPUT;
			break;
		case HFI_BUFFER_OUTPUT2:
			memcpy(&buffreq->buffer[2], hfi_buf_req,
				sizeof(struct hfi_buffer_requirements));
			buffreq->buffer[2].buffer_type = HAL_BUFFER_OUTPUT2;
			break;
		case HFI_BUFFER_EXTRADATA_INPUT:
			memcpy(&buffreq->buffer[3], hfi_buf_req,
				sizeof(struct hfi_buffer_requirements));
			buffreq->buffer[3].buffer_type =
				HAL_BUFFER_EXTRADATA_INPUT;
			break;
		case HFI_BUFFER_EXTRADATA_OUTPUT:
			memcpy(&buffreq->buffer[4], hfi_buf_req,
				sizeof(struct hfi_buffer_requirements));
			buffreq->buffer[4].buffer_type =
				HAL_BUFFER_EXTRADATA_OUTPUT;
			break;
		case HFI_BUFFER_EXTRADATA_OUTPUT2:
			memcpy(&buffreq->buffer[5], hfi_buf_req,
				sizeof(struct hfi_buffer_requirements));
			buffreq->buffer[5].buffer_type =
				HAL_BUFFER_EXTRADATA_OUTPUT2;
			break;
		case HFI_BUFFER_INTERNAL_SCRATCH:
			memcpy(&buffreq->buffer[6], hfi_buf_req,
			sizeof(struct hfi_buffer_requirements));
			buffreq->buffer[6].buffer_type =
				HAL_BUFFER_INTERNAL_SCRATCH;
			break;
		case HFI_BUFFER_INTERNAL_PERSIST:
			memcpy(&buffreq->buffer[7], hfi_buf_req,
			sizeof(struct hfi_buffer_requirements));
			buffreq->buffer[7].buffer_type =
				HAL_BUFFER_INTERNAL_PERSIST;
			break;
		default:
			HAL_MSG_ERROR("hal_process_sess_get_prop_buf_req:"
			"bad_buffer_type: %d", hfi_buf_req->buffer);
			break;
		}
		req_bytes -= sizeof(struct hfi_buffer_requirements);
		hfi_buf_req++;
	}
}

static void hal_process_session_prop_info(struct hal_device *device,
	struct hfi_msg_session_property_info_packet *pkt)
{
	struct msm_vidc_cb_cmd_done cmd_done;
	struct buffer_requirements buff_req;

	HAL_MSG_INFO("Received SESSION_PROPERTY_INFO");

	if (pkt->size < sizeof(struct hfi_msg_session_property_info_packet)) {
		HAL_MSG_ERROR("hal_process_session_prop_info:bad_pkt_size");
		return;
	}

	if (pkt->num_properties == 0) {
		HAL_MSG_ERROR("hal_process_session_prop_info:no_properties");
		return;
	}

	memset(&cmd_done, 0, sizeof(struct msm_vidc_cb_cmd_done));
	memset(&buff_req, 0, sizeof(struct buffer_requirements));

	switch (pkt->rg_property_data[0]) {
	case HFI_PROPERTY_CONFIG_BUFFER_REQUIREMENTS:
		hal_process_sess_get_prop_buf_req(pkt, &buff_req);
		cmd_done.device_id = device->device_id;
		cmd_done.session_id =
			((struct hal_session *) pkt->session_id)->session_id;
		cmd_done.status = VIDC_ERR_NONE;
		cmd_done.data = &buff_req;
		cmd_done.size = sizeof(struct buffer_requirements);
		device->callback(SESSION_PROPERTY_INFO, &cmd_done);
		break;
	default:
		HAL_MSG_ERROR("hal_process_session_prop_info:"
					"unknown_prop_id: %d",
				pkt->rg_property_data[0]);
		break;
	}
}

static void hal_process_session_init_done(struct hal_device *device,
	struct hfi_msg_sys_session_init_done_packet *pkt)
{
	struct msm_vidc_cb_cmd_done cmd_done;
	struct vidc_hal_session_init_done session_init_done;

	HAL_MSG_LOW("RECEIVED:SESSION_INIT_DONE");
	if (sizeof(struct hfi_msg_sys_session_init_done_packet)
		> pkt->size) {
		HAL_MSG_ERROR("hal_process_session_init_done:bad_pkt_size");
		return;
	}

	memset(&cmd_done, 0, sizeof(struct msm_vidc_cb_cmd_done));
	memset(&session_init_done, 0, sizeof(struct
				vidc_hal_session_init_done));

	cmd_done.device_id = device->device_id;
	cmd_done.session_id =
		((struct hal_session *) pkt->session_id)->session_id;
	cmd_done.status = vidc_map_hal_err_status((u32)pkt->error_type);
	cmd_done.data = &session_init_done;
	if (!cmd_done.status) {
		cmd_done.status = vidc_hal_process_sess_init_done_prop_read(
			pkt, &cmd_done);
	}
	cmd_done.size = sizeof(struct vidc_hal_session_init_done);
	device->callback(SESSION_INIT_DONE, &cmd_done);
}

static void hal_process_session_load_res_done(struct hal_device *device,
	struct hfi_msg_session_load_resources_done_packet *pkt)
{
	struct msm_vidc_cb_cmd_done cmd_done;
	HAL_MSG_LOW("RECEIVED:SESSION_LOAD_RESOURCES_DONE");

	if (sizeof(struct hfi_msg_session_load_resources_done_packet) !=
		pkt->size) {
		HAL_MSG_ERROR("hal_process_session_load_res_done:"
		" bad packet size: %d", pkt->size);
		return;
	}

	memset(&cmd_done, 0, sizeof(struct msm_vidc_cb_cmd_done));

	cmd_done.device_id = device->device_id;
	cmd_done.session_id =
		((struct hal_session *) pkt->session_id)->session_id;
	cmd_done.status = vidc_map_hal_err_status((u32)pkt->error_type);
	cmd_done.data = NULL;
	cmd_done.size = 0;
	device->callback(SESSION_LOAD_RESOURCE_DONE, &cmd_done);
}

static void hal_process_session_flush_done(struct hal_device *device,
	struct hfi_msg_session_flush_done_packet *pkt)
{
	struct msm_vidc_cb_cmd_done cmd_done;

	HAL_MSG_LOW("RECEIVED:SESSION_FLUSH_DONE");

	if (sizeof(struct hfi_msg_session_flush_done_packet) != pkt->size) {
		HAL_MSG_ERROR("hal_process_session_flush_done: "
		"bad packet size: %d", pkt->size);
		return;
	}

	memset(&cmd_done, 0, sizeof(struct msm_vidc_cb_cmd_done));
	cmd_done.device_id = device->device_id;
	cmd_done.session_id =
		((struct hal_session *) pkt->session_id)->session_id;
	cmd_done.status = vidc_map_hal_err_status((u32)pkt->error_type);
	cmd_done.data = (void *) pkt->flush_type;
	cmd_done.size = sizeof(u32);
	device->callback(SESSION_FLUSH_DONE, &cmd_done);
}

static void hal_process_session_etb_done(struct hal_device *device,
	struct hfi_msg_session_empty_buffer_done_packet *pkt)
{
	struct msm_vidc_cb_data_done data_done;

	HAL_MSG_LOW("RECEIVED:SESSION_ETB_DONE");

	if (!pkt || pkt->size !=
		sizeof(struct hfi_msg_session_empty_buffer_done_packet)) {
		HAL_MSG_ERROR("hal_process_session_etb_done:bad_pkt_size");
		return;
	}

	memset(&data_done, 0, sizeof(struct msm_vidc_cb_data_done));

	data_done.device_id = device->device_id;
	data_done.session_id =
		((struct hal_session *) pkt->session_id)->session_id;
	data_done.status = vidc_map_hal_err_status((u32) pkt->error_type);
	data_done.size = sizeof(struct msm_vidc_cb_data_done);
	data_done.clnt_data = (void *)pkt->input_tag;
	data_done.input_done.offset = pkt->offset;
	data_done.input_done.filled_len = pkt->filled_len;
	data_done.input_done.packet_buffer = pkt->packet_buffer;
	device->callback(SESSION_ETB_DONE, &data_done);
}

static void hal_process_session_ftb_done(struct hal_device *device,
			void *msg_hdr)
{
	struct msm_vidc_cb_data_done data_done;
	struct hfi_msg_session_fill_buffer_done_compressed_packet *pack =
	(struct hfi_msg_session_fill_buffer_done_compressed_packet *) msg_hdr;
	u32 is_decoder = ((struct hal_session *)pack->session_id)->is_decoder;
	struct hal_session *session;

	if (!msg_hdr) {
		HAL_MSG_ERROR("Invalid Params in %s", __func__);
		return;
	}

	session = (struct hal_session *)
		((struct hal_session *)	pack->session_id)->session_id;
	HAL_MSG_ERROR("RECEIVED:SESSION_FTB_DONE");

	memset(&data_done, 0, sizeof(struct msm_vidc_cb_data_done));

	if (is_decoder == 0) {
		struct hfi_msg_session_fill_buffer_done_compressed_packet *pkt =
		(struct hfi_msg_session_fill_buffer_done_compressed_packet *)
		msg_hdr;
		if (sizeof(struct
			hfi_msg_session_fill_buffer_done_compressed_packet)
			!= pkt->size) {
			HAL_MSG_ERROR("hal_process_session_ftb_done:"
						"bad_pkt_size");
			return;
		}

		data_done.device_id = device->device_id;
		data_done.session_id = (u32) session;
		data_done.status = vidc_map_hal_err_status((u32)
							pkt->error_type);
		data_done.size = sizeof(struct msm_vidc_cb_data_done);
		data_done.clnt_data = (void *) pkt->input_tag;

		data_done.output_done.timestamp_hi = pkt->timestamp_hi;
		data_done.output_done.timestamp_lo = pkt->timestamp_lo;
		data_done.output_done.flags1 = pkt->flags;
		data_done.output_done.mark_target = pkt->mark_target;
		data_done.output_done.mark_data = pkt->mark_data;
		data_done.output_done.stats = pkt->stats;
		data_done.output_done.offset1 = pkt->offset;
		data_done.output_done.alloc_len1 = pkt->alloc_len;
		data_done.output_done.filled_len1 = pkt->filled_len;
		data_done.output_done.picture_type = pkt->picture_type;
		data_done.output_done.packet_buffer1 = pkt->packet_buffer;
		data_done.output_done.extra_data_buffer =
			pkt->extra_data_buffer;
	} else if (is_decoder == 1) {
		struct hfi_msg_session_fbd_uncompressed_plane0_packet *pkt =
		(struct	hfi_msg_session_fbd_uncompressed_plane0_packet *)
		msg_hdr;
		if (sizeof(struct
		hfi_msg_session_fbd_uncompressed_plane0_packet)
		> pkt->size) {
			HAL_MSG_ERROR("hal_process_session_ftb_done:"
						"bad_pkt_size");
			return;
		}

		data_done.device_id = device->device_id;
		data_done.session_id = (u32) session;
		data_done.status = vidc_map_hal_err_status((u32)
			pkt->error_type);
		data_done.size = sizeof(struct msm_vidc_cb_data_done);
		data_done.clnt_data = (void *)pkt->input_tag;

		data_done.output_done.stream_id = pkt->stream_id;
		data_done.output_done.view_id = pkt->view_id;
		data_done.output_done.timestamp_hi = pkt->timestamp_hi;
		data_done.output_done.timestamp_lo = pkt->timestamp_lo;
		data_done.output_done.flags1 = pkt->flags;
		data_done.output_done.mark_target = pkt->mark_target;
		data_done.output_done.mark_data = pkt->mark_data;
		data_done.output_done.stats = pkt->stats;
		data_done.output_done.alloc_len1 = pkt->alloc_len;
		data_done.output_done.filled_len1 = pkt->filled_len;
		data_done.output_done.offset1 = pkt->oofset;
		data_done.output_done.frame_width = pkt->frame_width;
		data_done.output_done.frame_height = pkt->frame_height;
		data_done.output_done.start_xCoord = pkt->start_xCoord;
		data_done.output_done.start_yCoord = pkt->start_yCoord;
		data_done.output_done.input_tag1 = pkt->input_tag1;
		data_done.output_done.picture_type = pkt->picture_type;
		data_done.output_done.packet_buffer1 = pkt->packet_buffer;
		data_done.output_done.extra_data_buffer =
			pkt->extra_data_buffer;

		if (pkt->stream_id == 0)
			data_done.output_done.buffer_type = HAL_BUFFER_OUTPUT;
		else if (pkt->stream_id == 1)
			data_done.output_done.buffer_type = HAL_BUFFER_OUTPUT2;
		}
	device->callback(SESSION_FTB_DONE, &data_done);
}

static void hal_process_session_start_done(struct hal_device *device,
	struct hfi_msg_session_start_done_packet *pkt)
{
	struct msm_vidc_cb_cmd_done cmd_done;

	HAL_MSG_LOW("RECEIVED:SESSION_START_DONE");

	if (!pkt || pkt->size !=
		sizeof(struct hfi_msg_session_start_done_packet)) {
		HAL_MSG_ERROR("hal_process_session_start_done:"
		"bad packet/packet size: %d", pkt->size);
		return;
	}

	memset(&cmd_done, 0, sizeof(struct msm_vidc_cb_cmd_done));
	cmd_done.device_id = device->device_id;
	cmd_done.session_id =
		((struct hal_session *) pkt->session_id)->session_id;
	cmd_done.status = vidc_map_hal_err_status((u32)pkt->error_type);
	cmd_done.data = NULL;
	cmd_done.size = 0;
	device->callback(SESSION_START_DONE, &cmd_done);
}

static void hal_process_session_stop_done(struct hal_device *device,
	struct hfi_msg_session_stop_done_packet *pkt)
{
	struct msm_vidc_cb_cmd_done cmd_done;

	HAL_MSG_LOW("RECEIVED:SESSION_STOP_DONE");

	if (!pkt || pkt->size !=
		sizeof(struct hfi_msg_session_stop_done_packet)) {
		HAL_MSG_ERROR("hal_process_session_stop_done:"
		"bad packet/packet size: %d", pkt->size);
		return;
	}

	memset(&cmd_done, 0, sizeof(struct msm_vidc_cb_cmd_done));
	cmd_done.device_id = device->device_id;
	cmd_done.session_id =
		((struct hal_session *) pkt->session_id)->session_id;
	cmd_done.status = vidc_map_hal_err_status((u32)pkt->error_type);
	cmd_done.data = NULL;
	cmd_done.size = 0;
	device->callback(SESSION_STOP_DONE, &cmd_done);
}

static void hal_process_session_rel_res_done(struct hal_device *device,
	struct hfi_msg_session_release_resources_done_packet *pkt)
{
	struct msm_vidc_cb_cmd_done cmd_done;

	HAL_MSG_LOW("RECEIVED:SESSION_RELEASE_RESOURCES_DONE");

	if (!pkt || pkt->size !=
		sizeof(struct hfi_msg_session_release_resources_done_packet)) {
		HAL_MSG_ERROR("hal_process_session_rel_res_done:"
		"bad packet/packet size: %d", pkt->size);
		return;
	}

	memset(&cmd_done, 0, sizeof(struct msm_vidc_cb_cmd_done));
	cmd_done.device_id = device->device_id;
	cmd_done.session_id =
		((struct hal_session *) pkt->session_id)->session_id;
	cmd_done.status = vidc_map_hal_err_status((u32)pkt->error_type);
	cmd_done.data = NULL;
	cmd_done.size = 0;
	device->callback(SESSION_RELEASE_RESOURCE_DONE, &cmd_done);
}

static void hal_process_session_end_done(struct hal_device *device,
	struct hfi_msg_sys_session_end_done_packet *pkt)
{
	struct msm_vidc_cb_cmd_done cmd_done;
	struct list_head *curr, *next;
	struct hal_session *sess_close;

	HAL_MSG_LOW("RECEIVED:SESSION_END_DONE");

	if (!pkt || pkt->size !=
		sizeof(struct hfi_msg_sys_session_end_done_packet)) {
		HAL_MSG_ERROR("hal_process_session_end_done: "
		"bad packet/packet size: %d", pkt->size);
		return;
	}

	list_for_each_safe(curr, next, &device->sess_head) {
		sess_close = list_entry(curr, struct hal_session, list);
		HAL_MSG_MEDIUM("deleted the session: 0x%x",
					   sess_close->session_id);
		list_del(&sess_close->list);
		kfree(sess_close);
	}

	memset(&cmd_done, 0, sizeof(struct msm_vidc_cb_cmd_done));
	cmd_done.device_id = device->device_id;
	cmd_done.session_id =
		((struct hal_session *) pkt->session_id)->session_id;
	cmd_done.status = vidc_map_hal_err_status((u32)pkt->error_type);
	cmd_done.data = NULL;
	cmd_done.size = 0;
	device->callback(SESSION_END_DONE, &cmd_done);
}

static void hal_process_msg_packet(struct hal_device *device,
	struct vidc_hal_msg_pkt_hdr *msg_hdr)
{
	if (!device || !msg_hdr || msg_hdr->size <
		VIDC_IFACEQ_MIN_PKT_SIZE) {
		HAL_MSG_ERROR("hal_process_msg_packet:bad"
			"packet/packet size: %d", msg_hdr->size);
		return;
	}

	HAL_MSG_INFO("Received: 0x%x in %s", msg_hdr->packet, __func__);

	switch (msg_hdr->packet) {
	case HFI_MSG_EVENT_NOTIFY:
		hal_process_event_notify(device,
			(struct hfi_msg_event_notify_packet *) msg_hdr);
		break;
	case  HFI_MSG_SYS_INIT_DONE:
		hal_process_sys_init_done(device,
			(struct hfi_msg_sys_init_done_packet *)
					msg_hdr);
		break;
	case HFI_MSG_SYS_SESSION_INIT_DONE:
		hal_process_session_init_done(device,
			(struct hfi_msg_sys_session_init_done_packet *)
					msg_hdr);
		break;
	case HFI_MSG_SYS_SESSION_END_DONE:
		hal_process_session_end_done(device,
			(struct hfi_msg_sys_session_end_done_packet *)
					msg_hdr);
		break;
	case HFI_MSG_SESSION_LOAD_RESOURCES_DONE:
		hal_process_session_load_res_done(device,
			(struct hfi_msg_session_load_resources_done_packet *)
					msg_hdr);
		break;
	case HFI_MSG_SESSION_START_DONE:
		hal_process_session_start_done(device,
			(struct hfi_msg_session_start_done_packet *)
					msg_hdr);
		break;
	case HFI_MSG_SESSION_STOP_DONE:
		hal_process_session_stop_done(device,
			(struct hfi_msg_session_stop_done_packet *)
					msg_hdr);
		break;
	case HFI_MSG_SESSION_EMPTY_BUFFER_DONE:
		hal_process_session_etb_done(device,
			(struct hfi_msg_session_empty_buffer_done_packet *)
					msg_hdr);
		break;
	case HFI_MSG_SESSION_FILL_BUFFER_DONE:
		hal_process_session_ftb_done(device, msg_hdr);
		break;
	case HFI_MSG_SESSION_FLUSH_DONE:
		hal_process_session_flush_done(device,
			(struct hfi_msg_session_flush_done_packet *)
					msg_hdr);
		break;
	case HFI_MSG_SESSION_PROPERTY_INFO:
		hal_process_session_prop_info(device,
			(struct hfi_msg_session_property_info_packet *)
					msg_hdr);
		break;
	case HFI_MSG_SESSION_RELEASE_RESOURCES_DONE:
		hal_process_session_rel_res_done(device,
			(struct hfi_msg_session_release_resources_done_packet *)
					msg_hdr);
		break;
	default:
		HAL_MSG_ERROR("UNKNOWN_MSG_TYPE : %d", msg_hdr->packet);
		break;
	}
}

void vidc_hal_response_handler(struct hal_device *device)
{
	u8 packet[VIDC_IFACEQ_MED_PKT_SIZE];

	HAL_MSG_INFO("############vidc_hal_response_handler\n");
	if (device) {
		while (!vidc_hal_iface_msgq_read(device, packet)) {
			hal_process_msg_packet(device,
				(struct vidc_hal_msg_pkt_hdr *)	packet);
		}
	} else {
		HAL_MSG_ERROR("SPURIOUS_INTERRUPT");
	}
}
