/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "sample_comm_nnie.h"
#include "ai_infer_process.h"
#include "sample_media_ai.h"

#include <sys/prctl.h>

#include "sample_comm_nnie.h"
#include "sample_media_ai.h"
#include "ai_infer_process.h"
#include "vgs_img.h"
#include "ive_img.h"
#include "posix_help.h"
#include "audio_aac_adp.h"
#include "base_interface.h"
#include "osd_img.h"
#include "infant_detect.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define MODEL_FILE_INFANT    "/userdata/models/infant_detect/yolov3_10000_inst.wk" // darknet framework wk model
#define PIRIOD_NUM_MAX     49 // Logs are printed when the number of targets is detected
#define DETECT_OBJ_MAX     32 // detect max obj

#define AUDIO_SCORE        40

#define SCORE_MAX           4096    // The score corresponding to the maximum probability
#define DETECT_OBJ_MAX      32
#define RET_NUM_MAX         4
#define THRESH_MIN          30      // Acceptable probability threshold (over this value will be returned to the app)

#define FRM_WIDTH           256
#define FRM_HEIGHT          256
#define TXT_BEGX            20
#define TXT_BEGY            20

static int g_num = 108;
static int g_count = 0;
#define AUDIO_CASE_TWO     2
#define AUDIO_SCORE        40       // Confidence can be configured by yourself
#define AUDIO_FRAME        14       // Recognize once every 15 frames, can be configured by yourself

#define MULTIPLE_OF_EXPANSION 100   // Multiple of expansion
#define UNKOWN_WASTE          20    // Unkown Waste
#define BUFFER_SIZE           16    // buffer size
#define MIN_OF_BOX            16    // min of box
#define MAX_OF_BOX            240   // max of box

static HI_BOOL g_bAudioProcessStopSignal = HI_FALSE;
static pthread_t g_audioProcessThread = 0;
static OsdSet* g_osdsTrash = NULL;
static HI_S32 g_osd0Trash = -1;
DetectObjInfo infant[];



//static uintptr_t g_handModel = 0;
static uintptr_t g_infantModel = 0;
//1audio
static HI_VOID PlayAudio(const DetectObjInfo items)
{
    if  (g_count < AUDIO_FRAME) {
        g_count++;
        return;
    }

    const DetectObjInfo *item = &items;
    float score = item->score * MULTIPLE_OF_EXPANSION / SCORE_MAX;
    if ((score > AUDIO_SCORE) && (g_num != item->cls)) {
        g_num = item->cls;
        if (g_num != UNKOWN_WASTE) {
            AudioTest(g_num, -1);
        }
    }
    g_count = 0;
}
//2?SKPair
static SkPair g_stmChn = {
    .in = -1,
    .out = -1
};

static HI_VOID* GetAudioFileName(HI_VOID* arg)
{
    DetectObjInfo resBuf = {0};
    int ret;

    while (g_bAudioProcessStopSignal == false) {
        ret = FdReadMsg(g_stmChn.in, &resBuf, sizeof(DetectObjInfo));
        if (ret == sizeof(DetectObjInfo)) {
            PlayAudio(resBuf);
        }
    }

    return NULL;
}

HI_S32 InfantDetectLoad(uintptr_t* model)
{
    SAMPLE_SVP_NNIE_CFG_S *self = NULL;
    HI_S32 ret;

    ret = Yolo3Create(&self, MODEL_FILE_INFANT);
    *model = ret < 0 ? 0 : (uintptr_t)self;
    SAMPLE_PRT("Yolo3FdLoad ret:%d\n", ret);
    uartFd = UartOpenInit();
    if (uartFd < 0) {
        printf("uart1 open failed\r\n");
    } else {
        printf("uart1 open successed\r\n");
    }
    return ret;
}

 HI_S32 InfantDetectUnload(uintptr_t model)
{
    Yolo3Destory((SAMPLE_SVP_NNIE_CFG_S*)model);
    return 0;
}

static HI_S32 InfantDetect(uintptr_t model, IVE_IMAGE_S *srcYuv, DetectObjInfo resArr[])
{
    SAMPLE_SVP_NNIE_CFG_S *self = (SAMPLE_SVP_NNIE_CFG_S*)model;
    int objNum;
    int ret = Yolo3CalImg(self, srcYuv, resArr, DETECT_OBJ_MAX, &objNum);//boxs is the buffer store the results
    if (ret < 0) {
        SAMPLE_PRT("Infant detect Yolo3CalImg FAIL, for cal FAIL, ret:%d\n", ret);
        return ret;
    }
    if(resArr[0].cls==0)SAMPLE_PRT("The face is non-negative \n\n\n\n\n\n\n\n");
    if(resArr[0].cls==1)SAMPLE_PRT("The face is negative \n\n\n\n\n\n\n\n");

    return objNum;
}

HI_S32 InfantDetectCal(uintptr_t model,IVE_IMAGE_S *srcYuv, DetectObjInfo resArr[])
{
    int ret = InfantDetect(model, srcYuv, resArr);
    return ret;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
