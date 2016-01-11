/*
 * Copyright (C) 2016 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define CAMERA_PARAMETERS_EXTRA_C \
const char CameraParameters::KEY_MOT_AUTO_DC_OFFSET[] = "auto-dc-offset-mode"; \
const char CameraParameters::KEY_MOT_DENOISE[] = "mot-denoise-features"; \
const char CameraParameters::KEY_MOT_LUMA_ADAPTATION[] = "luma_adaptation"; \
const char CameraParameters::KEY_MOT_MANUAL_DC_OFFSET[] = "manual-dc-offset"; \
const char CameraParameters::KEY_MOT_TINTLESS[] = "tintless"; \
const char CameraParameters::KEY_MOT_SUPPORTED_AUTO_DC_OFFSET[] = "auto-dc-offset-mode-values"; \
const char CameraParameters::KEY_MOT_SUPPORTED_DENOISE[] = "mot-denoise-features-values"; \
const char CameraParameters::KEY_MOT_SUPPORTED_LUMA_ADAPTATION[] = "luma_adaptation-values"; \
const char CameraParameters::KEY_MOT_SUPPORTED_TINTLESS[] = "tintless-values"; \
const char CameraParameters::KEY_QC_AE_BRACKET_HDR[] = "ae-bracket-hdr"; \
const char CameraParameters::KEY_QC_AUTO_EXPOSURE[] = "auto-exposure"; \
const char CameraParameters::KEY_QC_CAMERA_MODE[] = "camera-mode"; \
const char CameraParameters::KEY_QC_CONTRAST[] = "contrast"; \
const char CameraParameters::KEY_QC_DENOISE[] = "denoise"; \
const char CameraParameters::KEY_QC_FACE_DETECTION[] = "face-detection"; \
const char CameraParameters::KEY_QC_HIGH_DYNAMIC_RANGE_IMAGING[] = "hdr"; \
const char CameraParameters::KEY_QC_ISO_MODE[] = "iso"; \
const char CameraParameters::KEY_QC_LENSSHADE[] = "lensshade"; \
const char CameraParameters::KEY_QC_MAX_CONTRAST[] = "max-contrast"; \
const char CameraParameters::KEY_QC_MAX_NUM_REQUESTED_FACES[] = "qc-max-num-requested-faces"; \
const char CameraParameters::KEY_QC_MAX_SATURATION[] = "max-saturation"; \
const char CameraParameters::KEY_QC_MAX_SHARPNESS[] = "max-sharpness"; \
const char CameraParameters::KEY_QC_MEMORY_COLOR_ENHANCEMENT[] = "mce"; \
const char CameraParameters::KEY_QC_PREVIEW_FRAME_RATE_AUTO_MODE[] = "frame-rate-auto"; \
const char CameraParameters::KEY_QC_PREVIEW_FRAME_RATE_FIXED_MODE[] = "frame-rate-fixed"; \
const char CameraParameters::KEY_QC_PREVIEW_FRAME_RATE_MODE[] = "preview-frame-rate-mode"; \
const char CameraParameters::KEY_QC_REDEYE_REDUCTION[] = "redeye-reduction"; \
const char CameraParameters::KEY_QC_SATURATION[] = "saturation"; \
const char CameraParameters::KEY_QC_SCENE_DETECT[] = "scene-detect"; \
const char CameraParameters::KEY_QC_SELECTABLE_ZONE_AF[] = "selectable-zone-af"; \
const char CameraParameters::KEY_QC_SHARPNESS[] = "sharpness"; \
const char CameraParameters::KEY_QC_SKIN_TONE_ENHANCEMENT[] = "skinToneEnhancement"; \
const char CameraParameters::KEY_QC_SUPPORTED_AUTO_EXPOSURE[] = "auto-exposure-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_CAMERA_FEATURES[] = "qc-camera-features"; \
const char CameraParameters::KEY_QC_SUPPORTED_DENOISE[] = "denoise-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_FACE_DETECTION[] = "face-detection-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_HDR_IMAGING_MODES[] = "hdr-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_HFR_SIZES[] = "hfr-size-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_ISO_MODES[] = "iso-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_LENSSHADE_MODES[] = "lensshade-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_MEM_COLOR_ENHANCE_MODES[] = "mce-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_PREVIEW_FRAME_RATE_MODES[] = "preview-frame-rate-modes"; \
const char CameraParameters::KEY_QC_SUPPORTED_REDEYE_REDUCTION[] = "redeye-reduction-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_SCENE_DETECT[] = "scene-detect-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_SELECTABLE_ZONE_AF[] = "selectable-zone-af-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_SKIN_TONE_ENHANCEMENT_MODES[] = "skinToneEnhancement-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_TOUCH_AF_AEC[] = "touch-af-aec-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_VIDEO_HIGH_FRAME_RATE_MODES[] = "video-hfr-values"; \
const char CameraParameters::KEY_QC_SUPPORTED_ZSL_MODES[] = "zsl-values"; \
const char CameraParameters::KEY_QC_TOUCH_AF_AEC[] = "touch-af-aec"; \
const char CameraParameters::KEY_QC_TOUCH_INDEX_AEC[] = "touch-index-aec"; \
const char CameraParameters::KEY_QC_TOUCH_INDEX_AF[] = "touch-index-af"; \
const char CameraParameters::KEY_QC_VIDEO_HIGH_FRAME_RATE[] = "video-hfr"; \
const char CameraParameters::KEY_QC_ZSL[] = "zsl"; \
\

#define CAMERA_PARAMETERS_EXTRA_H \
    static const char KEY_MOT_AUTO_DC_OFFSET[]; \
    static const char KEY_MOT_DENOISE[]; \
    static const char KEY_MOT_LUMA_ADAPTATION[]; \
    static const char KEY_MOT_MANUAL_DC_OFFSET[]; \
    static const char KEY_MOT_TINTLESS[]; \
    static const char KEY_MOT_SUPPORTED_AUTO_DC_OFFSET[]; \
    static const char KEY_MOT_SUPPORTED_DENOISE[]; \
    static const char KEY_MOT_SUPPORTED_LUMA_ADAPTATION[]; \
    static const char KEY_MOT_SUPPORTED_TINTLESS[]; \
    static const char KEY_QC_AE_BRACKET_HDR[]; \
    static const char KEY_QC_AUTO_EXPOSURE[]; \
    static const char KEY_QC_CAMERA_MODE[]; \
    static const char KEY_QC_CONTRAST[]; \
    static const char KEY_QC_DENOISE[]; \
    static const char KEY_QC_FACE_DETECTION[]; \
    static const char KEY_QC_HIGH_DYNAMIC_RANGE_IMAGING[]; \
    static const char KEY_QC_ISO_MODE[]; \
    static const char KEY_QC_LENSSHADE[]; \
    static const char KEY_QC_MAX_CONTRAST[]; \
    static const char KEY_QC_MAX_NUM_REQUESTED_FACES[]; \
    static const char KEY_QC_MAX_SATURATION[]; \
    static const char KEY_QC_MAX_SHARPNESS[]; \
    static const char KEY_QC_MEMORY_COLOR_ENHANCEMENT[]; \
    static const char KEY_QC_PREVIEW_FRAME_RATE_AUTO_MODE[]; \
    static const char KEY_QC_PREVIEW_FRAME_RATE_FIXED_MODE[]; \
    static const char KEY_QC_PREVIEW_FRAME_RATE_MODE[]; \
    static const char KEY_QC_REDEYE_REDUCTION[]; \
    static const char KEY_QC_SATURATION[]; \
    static const char KEY_QC_SCENE_DETECT[]; \
    static const char KEY_QC_SELECTABLE_ZONE_AF[]; \
    static const char KEY_QC_SHARPNESS[]; \
    static const char KEY_QC_SKIN_TONE_ENHANCEMENT[]; \
    static const char KEY_QC_SUPPORTED_AUTO_EXPOSURE[]; \
    static const char KEY_QC_SUPPORTED_CAMERA_FEATURES[]; \
    static const char KEY_QC_SUPPORTED_DENOISE[]; \
    static const char KEY_QC_SUPPORTED_FACE_DETECTION[]; \
    static const char KEY_QC_SUPPORTED_HDR_IMAGING_MODES[]; \
    static const char KEY_QC_SUPPORTED_HFR_SIZES[]; \
    static const char KEY_QC_SUPPORTED_ISO_MODES[]; \
    static const char KEY_QC_SUPPORTED_LENSSHADE_MODES[]; \
    static const char KEY_QC_SUPPORTED_MEM_COLOR_ENHANCE_MODES[]; \
    static const char KEY_QC_SUPPORTED_PREVIEW_FRAME_RATE_MODES[]; \
    static const char KEY_QC_SUPPORTED_REDEYE_REDUCTION[]; \
    static const char KEY_QC_SUPPORTED_SCENE_DETECT[]; \
    static const char KEY_QC_SUPPORTED_SELECTABLE_ZONE_AF[]; \
    static const char KEY_QC_SUPPORTED_SKIN_TONE_ENHANCEMENT_MODES[]; \
    static const char KEY_QC_SUPPORTED_TOUCH_AF_AEC[]; \
    static const char KEY_QC_SUPPORTED_VIDEO_HIGH_FRAME_RATE_MODES[]; \
    static const char KEY_QC_SUPPORTED_ZSL_MODES[]; \
    static const char KEY_QC_TOUCH_AF_AEC[]; \
    static const char KEY_QC_TOUCH_INDEX_AEC[]; \
    static const char KEY_QC_TOUCH_INDEX_AF[]; \
    static const char KEY_QC_VIDEO_HIGH_FRAME_RATE[]; \
    static const char KEY_QC_ZSL[]; \
    \
