CONFIG += c++17
wasm: DEFINES += NOAUDIO
android: DEFINES += NOAUDIO
DEFINES += NOAUDIO
!contains(DEFINES, NOAUDIO) {
QT += multimedia
SOURCES += \
    $$PWD/../libogg/bitwise.c \
    $$PWD/../libogg/framing.c \
    $$PWD/../opusfile/info.c \
    $$PWD/../opusfile/internal.c \
    $$PWD/../opusfile/opusfile.c \
    $$PWD/../opusfile/stream.c

SOURCES += \
    $$PWD/../libopus/silk/ana_filt_bank_1.c \
    $$PWD/../libopus/silk/inner_prod_aligned.c \
    $$PWD/../libopus/silk/CNG.c \
    $$PWD/../libopus/silk/process_NLSFs.c \
    $$PWD/../libopus/silk/init_decoder.c \
    $$PWD/../libopus/silk/tables_pulses_per_block.c \
    $$PWD/../libopus/silk/NLSF_VQ_weights_laroia.c \
    $$PWD/../libopus/silk/encode_pulses.c \
    $$PWD/../libopus/silk/resampler_rom.c \
    $$PWD/../libopus/silk/interpolate.c \
    $$PWD/../libopus/silk/NLSF_unpack.c \
    $$PWD/../libopus/silk/bwexpander.c \
    $$PWD/../libopus/silk/tables_LTP.c \
    $$PWD/../libopus/silk/NLSF_decode.c \
    $$PWD/../libopus/silk/LPC_fit.c \
    $$PWD/../libopus/silk/stereo_decode_pred.c \
    $$PWD/../libopus/silk/tables_gain.c \
    $$PWD/../libopus/silk/tables_NLSF_CB_NB_MB.c \
    $$PWD/../libopus/silk/stereo_quant_pred.c \
    $$PWD/../libopus/silk/NLSF_encode.c \
    $$PWD/../libopus/silk/sigm_Q15.c \
    $$PWD/../libopus/silk/resampler.c \
    $$PWD/../libopus/silk/float/encode_frame_FLP.c \
    $$PWD/../libopus/silk/float/pitch_analysis_core_FLP.c \
    $$PWD/../libopus/silk/float/residual_energy_FLP.c \
    $$PWD/../libopus/silk/float/corrMatrix_FLP.c \
    $$PWD/../libopus/silk/float/find_LTP_FLP.c \
    $$PWD/../libopus/silk/float/LTP_analysis_filter_FLP.c \
    $$PWD/../libopus/silk/float/find_pred_coefs_FLP.c \
    $$PWD/../libopus/silk/float/wrappers_FLP.c \
    $$PWD/../libopus/silk/float/schur_FLP.c \
    $$PWD/../libopus/silk/float/burg_modified_FLP.c \
    $$PWD/../libopus/silk/float/bwexpander_FLP.c \
    $$PWD/../libopus/silk/float/find_pitch_lags_FLP.c \
    $$PWD/../libopus/silk/float/autocorrelation_FLP.c \
    $$PWD/../libopus/silk/float/energy_FLP.c \
    $$PWD/../libopus/silk/float/warped_autocorrelation_FLP.c \
    $$PWD/../libopus/silk/float/regularize_correlations_FLP.c \
    $$PWD/../libopus/silk/float/LPC_analysis_filter_FLP.c \
    $$PWD/../libopus/silk/float/LTP_scale_ctrl_FLP.c \
    $$PWD/../libopus/silk/float/process_gains_FLP.c \
    $$PWD/../libopus/silk/float/k2a_FLP.c \
    $$PWD/../libopus/silk/float/noise_shape_analysis_FLP.c \
    $$PWD/../libopus/silk/float/apply_sine_window_FLP.c \
    $$PWD/../libopus/silk/float/find_LPC_FLP.c \
    $$PWD/../libopus/silk/float/scale_copy_vector_FLP.c \
    $$PWD/../libopus/silk/float/inner_product_FLP.c \
    $$PWD/../libopus/silk/float/scale_vector_FLP.c \
    $$PWD/../libopus/silk/float/sort_FLP.c \
    $$PWD/../libopus/silk/float/LPC_inv_pred_gain_FLP.c \
    $$PWD/../libopus/silk/NSQ_del_dec.c \
    $$PWD/../libopus/silk/NLSF_stabilize.c \
    $$PWD/../libopus/silk/stereo_encode_pred.c \
    $$PWD/../libopus/silk/check_control_input.c \
    $$PWD/../libopus/silk/sort.c \
    $$PWD/../libopus/silk/NSQ.c \
    $$PWD/../libopus/silk/dec_API.c \
    $$PWD/../libopus/silk/HP_variable_cutoff.c \
    $$PWD/../libopus/silk/VQ_WMat_EC.c \
    $$PWD/../libopus/silk/control_SNR.c \
    $$PWD/../libopus/silk/stereo_LR_to_MS.c \
    $$PWD/../libopus/silk/PLC.c \
    $$PWD/../libopus/silk/shell_coder.c \
    $$PWD/../libopus/silk/resampler_private_AR2.c \
    $$PWD/../libopus/silk/decode_core.c \
    $$PWD/../libopus/silk/resampler_down2_3.c \
    $$PWD/../libopus/silk/log2lin.c \
    $$PWD/../libopus/silk/NLSF_del_dec_quant.c \
    $$PWD/../libopus/silk/tables_other.c \
    $$PWD/../libopus/silk/init_encoder.c \
    $$PWD/../libopus/silk/tables_NLSF_CB_WB.c \
    $$PWD/../libopus/silk/decode_pitch.c \
    $$PWD/../libopus/silk/lin2log.c \
    $$PWD/../libopus/silk/NLSF_VQ.c \
    $$PWD/../libopus/silk/decode_indices.c \
    $$PWD/../libopus/silk/quant_LTP_gains.c \
    $$PWD/../libopus/silk/decode_pulses.c \
    $$PWD/../libopus/silk/LPC_inv_pred_gain.c \
    $$PWD/../libopus/silk/bwexpander_32.c \
    $$PWD/../libopus/silk/resampler_private_IIR_FIR.c \
    $$PWD/../libopus/silk/LP_variable_cutoff.c \
    $$PWD/../libopus/silk/A2NLSF.c \
    $$PWD/../libopus/silk/NLSF2A.c \
    $$PWD/../libopus/silk/pitch_est_tables.c \
    $$PWD/../libopus/silk/decode_frame.c \
    $$PWD/../libopus/silk/gain_quant.c \
    $$PWD/../libopus/silk/biquad_alt.c \
    $$PWD/../libopus/silk/control_audio_bandwidth.c \
    $$PWD/../libopus/silk/control_codec.c \
    $$PWD/../libopus/silk/decode_parameters.c \
    $$PWD/../libopus/silk/table_LSF_cos.c \
    $$PWD/../libopus/silk/encode_indices.c \
    $$PWD/../libopus/silk/code_signs.c \
    $$PWD/../libopus/silk/sum_sqr_shift.c \
    $$PWD/../libopus/silk/tables_pitch_lag.c \
    $$PWD/../libopus/silk/stereo_find_predictor.c \
    $$PWD/../libopus/silk/LPC_analysis_filter.c \
    $$PWD/../libopus/silk/resampler_private_up2_HQ.c \
    $$PWD/../libopus/silk/enc_API.c \
    $$PWD/../libopus/silk/stereo_MS_to_LR.c \
    $$PWD/../libopus/silk/resampler_down2.c \
    $$PWD/../libopus/silk/VAD.c \
    $$PWD/../libopus/silk/decoder_set_fs.c \
    $$PWD/../libopus/silk/resampler_private_down_FIR.c \
    $$PWD/../libopus/celt/celt_lpc.c \
    $$PWD/../libopus/celt/celt_encoder.c \
    $$PWD/../libopus/celt/pitch.c \
    $$PWD/../libopus/celt/celt.c \
    $$PWD/../libopus/celt/quant_bands.c \
    $$PWD/../libopus/celt/celt_decoder.c \
    $$PWD/../libopus/celt/kiss_fft.c \
    $$PWD/../libopus/celt/mathops.c \
    $$PWD/../libopus/celt/vq.c \
    $$PWD/../libopus/celt/laplace.c \
    $$PWD/../libopus/celt/bands.c \
    $$PWD/../libopus/celt/entenc.c \
    $$PWD/../libopus/celt/entdec.c \
    $$PWD/../libopus/celt/modes.c \
    $$PWD/../libopus/celt/mdct.c \
    $$PWD/../libopus/celt/rate.c \
    $$PWD/../libopus/celt/cwrs.c \
    $$PWD/../libopus/celt/entcode.c \
    $$PWD/../libopus/src/opus_multistream_decoder.c \
    $$PWD/../libopus/src/opus_multistream.c \
    $$PWD/../libopus/src/mlp_data.c \
    $$PWD/../libopus/src/opus_projection_decoder.c \
    $$PWD/../libopus/src/opus_decoder.c \
    $$PWD/../libopus/src/mapping_matrix.c \
    $$PWD/../libopus/src/mlp.c \
    $$PWD/../libopus/src/opus_projection_encoder.c \
    $$PWD/../libopus/src/analysis.c \
    $$PWD/../libopus/src/opus_compare.c \
    $$PWD/../libopus/src/opus_encoder.c \
    $$PWD/../libopus/src/repacketizer.c \
    $$PWD/../libopus/src/opus_multistream_encoder.c \
    $$PWD/../libopus/src/opus.c

HEADERS  += \
    $$PWD/../libogg/ogg.h \
    $$PWD/../libogg/os_types.h \
    $$PWD/../opusfile/internal.h \
    $$PWD/../opusfile/opusfile.h
    
    HEADERS  += \
    $$PWD/../libopus/include/opus.h \
    $$PWD/../libopus/include/opus_multistream.h \
    $$PWD/../libopus/include/opus_types.h \
    $$PWD/../libopus/include/opus_custom.h \
    $$PWD/../libopus/include/opus_projection.h \
    $$PWD/../libopus/include/opus_defines.h \
    $$PWD/../libopus/silk/structs.h \
    $$PWD/../libopus/silk/define.h \
    $$PWD/../libopus/silk/Inlines.h \
    $$PWD/../libopus/silk/API.h \
    $$PWD/../libopus/silk/pitch_est_defines.h \
    $$PWD/../libopus/silk/typedef.h \
    $$PWD/../libopus/silk/float/structs_FLP.h \
    $$PWD/../libopus/silk/float/SigProc_FLP.h \
    $$PWD/../libopus/silk/float/main_FLP.h \
    $$PWD/../libopus/silk/PLC.h \
    $$PWD/../libopus/silk/errors.h \
    $$PWD/../libopus/silk/resampler_rom.h \
    $$PWD/../libopus/silk/resampler_structs.h \
    $$PWD/../libopus/silk/tables.h \
    $$PWD/../libopus/silk/resampler_private.h \
    $$PWD/../libopus/silk/MacroCount.h \
    $$PWD/../libopus/silk/SigProc_FIX.h \
    $$PWD/../libopus/silk/control.h \
    $$PWD/../libopus/silk/NSQ.h \
    $$PWD/../libopus/silk/main.h \
    $$PWD/../libopus/silk/tuning_parameters.h \
    $$PWD/../libopus/silk/macros.h \
    $$PWD/../libopus/celt/ecintrin.h \
    $$PWD/../libopus/celt/modes.h \
    $$PWD/../libopus/celt/celt.h \
    $$PWD/../libopus/celt/arch.h \
    $$PWD/../libopus/celt/vq.h \
    $$PWD/../libopus/celt/entenc.h \
    $$PWD/../libopus/celt/entcode.h \
    $$PWD/../libopus/celt/static_modes_float.h \
    $$PWD/../libopus/celt/entdec.h \
    $$PWD/../libopus/celt/celt_lpc.h \
    $$PWD/../libopus/celt/float_cast.h \
    $$PWD/../libopus/celt/rate.h \
    $$PWD/../libopus/celt/bands.h \
    $$PWD/../libopus/celt/cwrs.h \
    $$PWD/../libopus/celt/fixed_generic.h \
    $$PWD/../libopus/celt/_kiss_fft_guts.h \
    $$PWD/../libopus/celt/pitch.h \
    $$PWD/../libopus/celt/mfrngcod.h \
    $$PWD/../libopus/celt/mdct.h \
    $$PWD/../libopus/celt/static_modes_fixed.h \
    $$PWD/../libopus/celt/cpu_support.h \
    $$PWD/../libopus/celt/kiss_fft.h \
    $$PWD/../libopus/celt/stack_alloc.h \
    $$PWD/../libopus/celt/laplace.h \
    $$PWD/../libopus/celt/mathops.h \
    $$PWD/../libopus/celt/quant_bands.h \
    $$PWD/../libopus/celt/os_support.h \
    $$PWD/../libopus/src/tansig_table.h \
    $$PWD/../libopus/src/opus_private.h \
    $$PWD/../libopus/src/mlp.h \
    $$PWD/../libopus/src/analysis.h \
    $$PWD/../libopus/src/mapping_matrix.h \
    $$PWD/../libopus/win32/config.h

    INCLUDEPATH += $$PWD/../libopus/include/
    #Opus requires one of VAR_ARRAYS, USE_ALLOCA, or NONTHREADSAFE_PSEUDOSTACK be defined to select the temporary allocation mode.
    DEFINES += USE_ALLOCA OPUS_BUILD
}

include(ultracopier-little.pri)

SOURCES += \
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngine-collision-and-error.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngine.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/DebugDialog.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/DiskSpace.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/DriveManagement.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/FileErrorDialog.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/FileExistsDialog.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/FileIsSameDialog.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/FilterRules.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/Filters.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/FolderExistsDialog.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThread_InodeAction.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/MkPath.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/RenamingRules.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ScanFileOrFolder.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/TransferThread.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThread.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/async/TransferThreadAsync.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadActions.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadStat.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadScan.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadOptions.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadNew.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadMedia.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadListChange.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/async/ReadThread.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/async/WriteThread.cpp

RESOURCES += \
    ../plugins/CopyEngine/Ultracopier-Spec/copyEngineResources.qrc

FORMS += \
    ../plugins/CopyEngine/Ultracopier-Spec/copyEngineOptions.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/debugDialog.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/DiskSpace.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/fileErrorDialog.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/fileExistsDialog.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/fileIsSameDialog.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/FilterRules.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/Filters.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/folderExistsDialog.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/RenamingRules.ui

HEADERS += \
    ../plugins/CopyEngine/Ultracopier-Spec/CompilerInfo.h \
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngine.h \
    ../plugins/CopyEngine/Ultracopier-Spec/DebugDialog.h \
    ../plugins/CopyEngine/Ultracopier-Spec/DebugEngineMacro.h \
    ../plugins/CopyEngine/Ultracopier-Spec/DiskSpace.h \
    ../plugins/CopyEngine/Ultracopier-Spec/DriveManagement.h \
    ../plugins/CopyEngine/Ultracopier-Spec/Environment.h \
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.h \
    ../plugins/CopyEngine/Ultracopier-Spec/FileErrorDialog.h \
    ../plugins/CopyEngine/Ultracopier-Spec/FileExistsDialog.h \
    ../plugins/CopyEngine/Ultracopier-Spec/FileIsSameDialog.h \
    ../plugins/CopyEngine/Ultracopier-Spec/FilterRules.h \
    ../plugins/CopyEngine/Ultracopier-Spec/Filters.h \
    ../plugins/CopyEngine/Ultracopier-Spec/FolderExistsDialog.h \
    ../plugins/CopyEngine/Ultracopier-Spec/MkPath.h \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThread.h \
    ../plugins/CopyEngine/Ultracopier-Spec/RenamingRules.h \
    ../plugins/CopyEngine/Ultracopier-Spec/ScanFileOrFolder.h \
    ../plugins/CopyEngine/Ultracopier-Spec/StructEnumDefinition_CopyEngine.h \
    ../plugins/CopyEngine/Ultracopier-Spec/StructEnumDefinition.h \
    ../plugins/CopyEngine/Ultracopier-Spec/TransferThread.h \
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngineUltracopier-SpecVariable.h \
    ../plugins/CopyEngine/Ultracopier-Spec/async/TransferThreadAsync.h \
    ../plugins/CopyEngine/Ultracopier-Spec/async/ReadThread.h \
    ../plugins/CopyEngine/Ultracopier-Spec/async/WriteThread.h

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/../android-sources

DISTFILES += \
    ../android-sources/AndroidManifest.xml
