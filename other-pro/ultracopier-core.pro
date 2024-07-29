CONFIG += c++17
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"
#QMAKE_CXXFLAGS+="-Wall -Wextra -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-newline-eof -Wno-exit-time-destructors -Wno-global-constructors -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation -Wno-shadow -Wno-missing-prototypes -Wno-padded -Wno-covered-switch-default -Wno-old-style-cast -Wno-documentation-unknown-command -Wno-switch-enum -Wno-undefined-reinterpret-cast -Wno-unreachable-code-break -Wno-sign-conversion -Wno-float-conversion"
QMAKE_CXXFLAGS+=""
DEFINES += _LARGE_FILE_SOURCE=1 _FILE_OFFSET_BITS=64 _UNICODE UNICODE

!contains(DEFINES, AUDIO) {
DEFINES += NOAUDIO
}
wasm: DEFINES += NOAUDIO
macx {
    DEFINES += NOAUDIO
}
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

TEMPLATE = app
QT += network xml widgets
TRANSLATIONS += $$PWD/../plugins/Languages/ar/translation.ts \
    $$PWD/../plugins/Languages/de/translation.ts \
    $$PWD/../plugins/Languages/el/translation.ts \
    $$PWD/../resources/Languages/en/translation.ts \
    $$PWD/../plugins/Languages/es/translation.ts \
    $$PWD/../plugins/Languages/fr/translation.ts \
    $$PWD/../plugins/Languages/hi/translation.ts \
    $$PWD/../plugins/Languages/hu/translation.ts \
    $$PWD/../plugins/Languages/id/translation.ts \
    $$PWD/../plugins/Languages/it/translation.ts \
    $$PWD/../plugins/Languages/ja/translation.ts \
    $$PWD/../plugins/Languages/ko/translation.ts \
    $$PWD/../plugins/Languages/nl/translation.ts \
    $$PWD/../plugins/Languages/no/translation.ts \
    $$PWD/../plugins/Languages/pl/translation.ts \
    $$PWD/../plugins/Languages/pt/translation.ts \
    $$PWD/../plugins/Languages/ru/translation.ts \
    $$PWD/../plugins/Languages/th/translation.ts \
    $$PWD/../plugins/Languages/tr/translation.ts \
    $$PWD/../plugins/Languages/zh/translation.ts \
    $$PWD/../plugins/Languages/zh_TW/translation.ts

TARGET = ultracopier
macx {
    ICON = $$PWD/../resources/ultracopier.icns
    #QT += macextras
    VERSION = 2.0.0.1
}
FORMS += $$PWD/../HelpDialog.ui \
    $$PWD/../PluginInformation.ui \
    $$PWD/../OptionDialog.ui \
    $$PWD/../OSSpecific.ui \
    $$PWD/../ProductKey.ui
RESOURCES += \
    $$PWD/../resources/ultracopier-resources.qrc \
    $$PWD/../resources/ultracopier-resources_unix.qrc \
    $$PWD/../resources/ultracopier-resources_windows.qrc
win32 {
    !contains(DEFINES, ULTRACOPIER_PLUGIN_ALL_IN_ONE) {
        RESOURCES += $$PWD/../resources/resources-windows-qt-plugin.qrc
    }
    RC_FILE += $$PWD/../resources/resources-windows.rc
    #LIBS += -lpdh
        LIBS += -ladvapi32
}

HEADERS += $$PWD/../ResourcesManager.h \
    $$PWD/../ThemesManager.h \
    $$PWD/../SystrayIcon.h \
    $$PWD/../StructEnumDefinition.h \
    $$PWD/../EventDispatcher.h \
    $$PWD/../Environment.h \
    $$PWD/../DebugEngine.h \
    $$PWD/../Core.h \
    $$PWD/../OptionEngine.h \
    $$PWD/../HelpDialog.h \
    $$PWD/../PluginsManager.h \
    $$PWD/../LanguagesManager.h \
    $$PWD/../DebugEngineMacro.h \
    $$PWD/../PluginInformation.h \
    $$PWD/../lib/qt-tar-xz/xz.h \
    $$PWD/../lib/qt-tar-xz/QXzDecodeThread.h \
    $$PWD/../lib/qt-tar-xz/QXzDecode.h \
    $$PWD/../lib/qt-tar-xz/QTarDecode.h \
    $$PWD/../SessionLoader.h \
    $$PWD/../ExtraSocket.h \
    $$PWD/../CopyListener.h \
    $$PWD/../CopyEngineManager.h \
    $$PWD/../PlatformMacro.h \
    $$PWD/../interface/PluginInterface_Themes.h \
    $$PWD/../interface/PluginInterface_SessionLoader.h \
    $$PWD/../interface/PluginInterface_Listener.h \
    $$PWD/../interface/PluginInterface_CopyEngine.h \
    $$PWD/../interface/OptionInterface.h \
    $$PWD/../Version.h \
    $$PWD/../Variable.h \
    $$PWD/../PluginLoaderCore.h \
    $$PWD/../interface/PluginInterface_PluginLoader.h \
    $$PWD/../OptionDialog.h \
    $$PWD/../LocalPluginOptions.h \
    $$PWD/../LocalListener.h \
    $$PWD/../CliParser.h \
    $$PWD/../interface/FacilityInterface.h \
    $$PWD/../FacilityEngine.h \
    $$PWD/../LogThread.h \
    $$PWD/../CompilerInfo.h \
    $$PWD/../StructEnumDefinition_UltracopierSpecific.h \
    $$PWD/../OSSpecific.h \
    $$PWD/../cpp11addition.h \
    $$PWD/../InternetUpdater.h \
    $$PWD/../ProductKey.h
SOURCES += $$PWD/../ThemesManager.cpp \
    $$PWD/../ResourcesManager.cpp \
    $$PWD/../main.cpp \
    $$PWD/../EventDispatcher.cpp \
    $$PWD/../SystrayIcon.cpp \
    $$PWD/../DebugEngine.cpp \
    $$PWD/../OptionEngine.cpp \
    $$PWD/../HelpDialog.cpp \
    $$PWD/../PluginsManager.cpp \
    $$PWD/../LanguagesManager.cpp \
    $$PWD/../PluginInformation.cpp \
    $$PWD/../lib/qt-tar-xz/QXzDecodeThread.cpp \
    $$PWD/../lib/qt-tar-xz/QXzDecode.cpp \
    $$PWD/../lib/qt-tar-xz/QTarDecode.cpp \
    $$PWD/../lib/qt-tar-xz/xz_crc32.c \
    $$PWD/../lib/qt-tar-xz/xz_dec_stream.c \
    $$PWD/../lib/qt-tar-xz/xz_dec_lzma2.c \
    $$PWD/../lib/qt-tar-xz/xz_dec_bcj.c \
    $$PWD/../SessionLoader.cpp \
    $$PWD/../ExtraSocket.cpp \
    $$PWD/../CopyListener.cpp \
    $$PWD/../CopyEngineManager.cpp \
    $$PWD/../Core.cpp \
    $$PWD/../PluginLoaderCore.cpp \
    $$PWD/../OptionDialog.cpp \
    $$PWD/../LocalPluginOptions.cpp \
    $$PWD/../LocalListener.cpp \
    $$PWD/../CliParser.cpp \
    $$PWD/../FacilityEngine.cpp \
    $$PWD/../FacilityEngineVersion.cpp \
    $$PWD/../LogThread.cpp \
    $$PWD/../OSSpecific.cpp \
    $$PWD/../cpp11addition.cpp \
    $$PWD/../DebugModel.cpp \
    $$PWD/../InternetUpdater.cpp \
    $$PWD/../cpp11additionstringtointcpp.cpp \
    $$PWD/../ProductKey.cpp
INCLUDEPATH += \
    $$PWD/../lib/qt-tar-xz/

OTHER_FILES += $$PWD/../resources/resources-windows.rc

win32: {
DEFINES += WIDESTRING
}
