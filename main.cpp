#include <yaal/io/ports.hh>
#include <yaal/io/serial.hh>
#include <yaal/communication/i2c_hw.hh>

#include <util/delay.h>

#define DEBUG 1
#define CALIBRATE 0
#define ENC_TEST_PATTERN 0

using namespace yaal;

namespace ad_decoder {
    enum InputSelection : uint8_t {
        INSEL_CVBS_Ain1      = 0x00,
        INSEL_CVBS_Ain2      = 0x01,
        INSEL_CVBS_Ain3      = 0x02,
        INSEL_CVBS_Ain4      = 0x03,
        INSEL_YC_Ain1_2      = 0x08,
        INSEL_YC_Ain3_4      = 0x09,
        INSEL_YPbPr_Ain1_2_3 = 0x0c,
    };

    enum AutoDetectSelection : uint8_t {
        AD_PALBGHID_NTSCJ_SECAM = 0x04,
        AD_PALBGHID_NTSCM_SECAM = 0x14,
        AD_PALN_NTSCJ_SECAM     = 0x24,
        AD_PALN_NTSCM_SECAM     = 0x34,
        AD_NTSCJ                = 0x44,
        AD_NTSCM                = 0x54,
        AD_PAL60                = 0x64,
        AD_NTSC443              = 0x74,
        AD_PALBGHID             = 0x84,
        AD_PALN                 = 0x94,
        AD_PALM                 = 0xa4,
        AD_PALM_P               = 0xb4,
        AD_PAL_COMBI_N          = 0xc4,
        AD_PAL_COMBI_N_P        = 0xd4,
        AD_SECAM                = 0xe4,
        AD_SECAM2               = 0xf4,
    };

    enum DecoderSubmap : uint8_t {
        DEC_SUBMAP_USER     = 0x00,
        DEC_SUBMAP_INTR_VDP = 0x20,
        DEC_SUBMAP_USER2    = 0x40,
        DEC_SUBMAP_0x80     = 0x80,
    };

    enum AlphaBlend : uint8_t {
        AB_SHARPEST  = 0x00,
        AB_SHARP     = 0x04,
        AB_SMOOTH    = 0x08,
        AB_SMOOTHEST = 0x0c,
    };

    enum I2P_Algorithm : uint8_t {
        I2P_ALG_LINEDOUBLE  = 0x00,
        I2P_ALG_DEINTERLACE = 0x18,
    };

    constexpr uint8_t OUTC_TOD    = 0x40;
    constexpr uint8_t OUTC_VBI_EN = 0x80;

    constexpr uint8_t EOUTC_RANGE_LIM   = 0x00;
    constexpr uint8_t EOUTC_RANGE_FULL  = 0x01;
    constexpr uint8_t EOUTC_SFL_EN      = 0x02;
    constexpr uint8_t EOUTC_BLANK_C_VBI = 0x04;
    constexpr uint8_t EOUTC_TIM_OE      = 0x08;
    constexpr uint8_t EOUTC_BT656_4     = 0x80;

    constexpr uint8_t PWRM_PWRDWN = 0x20;
    constexpr uint8_t PWRM_RESET  = 0x80;

    constexpr uint8_t CTIDNRC_CTI_EN    = 0x01;
    constexpr uint8_t CTIDNRC_CTI_AB_EN = 0x02;
    constexpr uint8_t CTIDNRC_DNR_EN    = 0x20;

    constexpr uint8_t AFEC_F1_EN   = 0x01;
    constexpr uint8_t AFEC_F2_EN   = 0x02;
    constexpr uint8_t AFEC_F3_EN   = 0x04;
    constexpr uint8_t AFEC_F4_EN   = 0x08;
    constexpr uint8_t AFEC_MAN_OVR = 0x10;

    template<typename INTRQ, typename RESET, typename PWRDWN>
    class ADV7280A {
    public:
        INTRQ intrq;
        RESET reset;
        PWRDWN pwrdwn;

        uint8_t address;
        uint8_t vpp_address;

        ADV7280A(uint8_t addr, uint8_t vpp_addr = 0x84)
            : address(addr), vpp_address(vpp_addr) {
            intrq.mode = INPUT_PULLUP;
            reset.mode = OUTPUT;
            reset = false;
            pwrdwn.mode = OUTPUT;
            pwrdwn = false;
        }

        void select_input(InputSelection input) {
            uint8_t insel[] = { 0x00, input };
            I2c_HW.write_multi(address, insel, insel + sizeof(insel));
        }

        void select_autodetection(AutoDetectSelection ad) {
            uint8_t autodetect[] = { 0x02, ad };
            I2c_HW.write_multi(address, autodetect, autodetect + sizeof(autodetect));
        }

        void select_submap(DecoderSubmap sm) {
            uint8_t submap[] = { 0x0e, sm };
            I2c_HW.write_multi(address, submap, submap + sizeof(submap));
        }

        void set_output_control(bool tristate_outputs, bool enable_vbi) {
            uint8_t outc[] = { 0x03, 0x0c };
            if (tristate_outputs)
                outc[1] |= OUTC_TOD;
            if (enable_vbi)
                outc[1] |= OUTC_VBI_EN;
            I2c_HW.write_multi(address, outc, outc + sizeof(outc));
        }

        void set_ext_output_control(bool full_range, bool enable_sfl,
                bool blank_chroma_vbi, bool enable_timing_out, bool bt656_4) {
            uint8_t ext_outc[] = { 0x04, 0x30 };
            if (full_range)
                ext_outc[1] |= EOUTC_RANGE_FULL;
            if (enable_sfl)
                ext_outc[1] |= EOUTC_SFL_EN;
            if (blank_chroma_vbi)
                ext_outc[1] |= EOUTC_BLANK_C_VBI;
            if (enable_timing_out)
                ext_outc[1] |= EOUTC_TIM_OE;
            if (bt656_4)
                ext_outc[1] |= EOUTC_BT656_4;
            I2c_HW.write_multi(address, ext_outc, ext_outc + sizeof(ext_outc));
        }

        void set_power_management(bool powerdown, bool reset) {
            uint8_t pwr_mgmt[] = { 0x0f, 0x00 };
            if (powerdown)
                pwr_mgmt[1] |= PWRM_PWRDWN;
            if (reset)
                pwr_mgmt[1] |= PWRM_RESET;
            I2c_HW.write_multi(address, pwr_mgmt, pwr_mgmt + sizeof(pwr_mgmt));
        }

        void set_cti_dnr_control(bool enable_cti, bool enable_cti_ab, AlphaBlend ab, bool enable_dnr) {
            // CTI DNR control
            // Disable CTI and CTI alpha blender
            uint8_t cti_dnr[] = { 0x4d, 0xc0 };
            if (enable_cti)
                cti_dnr[1] |= CTIDNRC_CTI_EN;
            if (enable_cti_ab)
                cti_dnr[1] |= CTIDNRC_CTI_AB_EN;
            cti_dnr[1] |= ab;
            if (enable_dnr)
                cti_dnr[1] |= CTIDNRC_DNR_EN;
            I2c_HW.write_multi(address, cti_dnr, cti_dnr + sizeof(cti_dnr));
        }

        void deinterlace_reset() {
            // TODO
            // VPP slave address: User sub map, subaddress 0xfd, bits 6:0
            // VPP Map, subaddress 0x41, bit 0
        }

        void deinterlace_control(bool enable,
                I2P_Algorithm alg = I2P_ALG_DEINTERLACE) {
            // TODO
            // VPP slave address: User sub map, subaddress 0xfd, bits 6:0
            // Deinterlace enable: VPP Map, subaddress 0x55, bit 7
            // Algorithm selection (undocumented): VPP Map, subaddress 0x5a, bits 4:3
            //     0b00: Simple line doubling
            //     0b11: Proprietary deinterlacing algorithm
            // Advanced timing mode: VPP Map, subaddress 0x5b, bit 7 (negative)
        }

        void set_aa_filters(bool man_ovr, bool f1, bool f2, bool f3, bool f4) {
            uint8_t afec[] = { 0xf3, 0x00 };
            if (man_ovr)
                afec[1] |= AFEC_MAN_OVR;
            if (f1)
                afec[1] |= AFEC_F1_EN;
            if (f2)
                afec[1] |= AFEC_F2_EN;
            if (f3)
                afec[1] |= AFEC_F3_EN;
            if (f4)
                afec[1] |= AFEC_F4_EN;
            I2c_HW.write_multi(address, afec, afec + sizeof(afec));
        }
    };
}

template<typename RESET>
class ADV7391 {
public:
    RESET reset;

    unsigned char address;

    ADV7391(unsigned char addr) : address(addr) {
        reset.mode = OUTPUT;
        reset = false;
    }
};

using namespace ad_decoder;

ADV7280A<PortD2, PortD6, PortC2> decoder(0x20);
ADV7391<PortD7> encoder(0x2a);

PortC4 sda;
PortC5 scl;

#if DEBUG
Serial0 serial;
#endif

enum {
    CVBS,
    CVBS_PEDESTAL,
    SVIDEO,
    SVIDEO_PEDESTAL,
} curr_input;

PortD5 input_change;
volatile bool input_change_down = false;

static inline bool read_input_change()
{
    cli();
    const bool ret = input_change_down;
    input_change_down = false;
    sei();
    return ret;
}

static inline void input_change_debounce()
{
    static uint8_t counter = 0;
    static bool state = false;

    bool curr_state = !input_change;

    if (curr_state != state) {
        if (++counter == 4) {
            state = curr_state;
            if (state) {
                // Will be reset to false when read
                // in read_input_change().
                input_change_down = true;
            }
            counter = 0;
        }
    }
    else {
        counter = 0;
    }
}

PortB7 option;
volatile bool option_down = false;

static inline bool read_option()
{
    cli();
    const bool ret = option_down;
    option_down = false;
    sei();
    return ret;
}

static inline void option_debounce()
{
    static uint8_t counter = 0;
    static bool state = false;

    bool curr_state = !option;

    if (curr_state != state) {
        if (++counter == 4) {
            state = curr_state;
            if (state) {
                // Will be reset to false when read
                // in read_option().
                option_down = true;
            }
            counter = 0;
        }
    }
    else {
        counter = 0;
    }
}

// ISR to run debouncing every 10 ms
ISR(TIMER0_COMPA_vect)
{
    input_change_debounce();
    option_debounce();
}

static void setup_timer0()
{
    // CTC mode, prescaler 1024, target frequency 100 Hz
    TCCR0A = _BV(WGM01);
    TCCR0B = _BV(CS00) | _BV(CS02);
    OCR0A = 4;

    // Generate an interrupt on compare match
    TIMSK0 = _BV(OCIE0A);
}

static void setup_encoder()
{
#if 0
    // All DACs and PLL enabled
    uint8_t seq_1_1[] = { 0x00, 0x1c };
    I2c_HW.write_multi(encoder.address, seq_1_1, seq_1_1 + sizeof(seq_1_1));
#else
    // All DACs enabled, PLL disabled (only 2x oversampling)
    uint8_t seq_1_1[] = { 0x00, 0x1e };
    I2c_HW.write_multi(encoder.address, seq_1_1, seq_1_1 + sizeof(seq_1_1));
#endif

#if !ENC_TEST_PATTERN
    // SD input mode
    uint8_t seq_1_2[] = { 0x01, 0x00 };
    I2c_HW.write_multi(encoder.address, seq_1_2, seq_1_2 + sizeof(seq_1_2));

    // NTSC, SSAF luma filter, 1.3MHz chroma filter
    uint8_t seq_1_3[] = { 0x80, 0x10 };
    I2c_HW.write_multi(encoder.address, seq_1_3, seq_1_3 + sizeof(seq_1_3));
#endif

#if 0
    // Pixel data valid, YPrPb, PrPb SSAF filter, AVE control, pedestal
    uint8_t seq_1_4[] = { 0x82, 0xc9 };
    I2c_HW.write_multi(encoder.address, seq_1_4, seq_1_4 + sizeof(seq_1_4));
#else
    // Pixel data valid, YPrPb, *no* PrPb SSAF filter, AVE control, pedestal
    uint8_t seq_1_4[] = { 0x82, 0xc8 };
    I2c_HW.write_multi(encoder.address, seq_1_4, seq_1_4 + sizeof(seq_1_4));
#endif

#if 1
    // Enable VBI. Otherwise default.
    uint8_t sdm3[] = { 0x83, 0x14 };
    I2c_HW.write_multi(encoder.address, sdm3, sdm3 + sizeof(sdm3));
#endif

    // Enable subcarrier frequency lock
    uint8_t sdm4[] = { 0x84, 0x06 };
    I2c_HW.write_multi(encoder.address, sdm4, sdm4 + sizeof(sdm4));

    // Autodetect SD input standard
    uint8_t sdm6[] = { 0x87, 0x20 };
    I2c_HW.write_multi(encoder.address, sdm6, sdm6 + sizeof(sdm6));

#if !ENC_TEST_PATTERN
    // Enable SD progressive mode (for allowing 240p/288p)
    uint8_t sdm7[] = { 0x88, 0x02 };
    I2c_HW.write_multi(encoder.address, sdm7, sdm7 + sizeof(sdm7));
#endif

#if ENC_TEST_PATTERN
    // Color bar test pattern
    uint8_t pleasework1[] = { 0x84, 0x40 };
    I2c_HW.write_multi(encoder.address, pleasework1, pleasework1 + 2);
#endif
}

static inline void setup_ad_black_magic()
{
    // Undocumented black magic from AD scripts:
    // 42 0E 80 ; ADI Required Write
    // 42 9C 00 ; Reset Current Clamp Circuitry [step1]
    // 42 9C FF ; Reset Current Clamp Circuitry [step2]
    // 42 0E 00 ; Enter User Sub Map
    // 42 80 51 ; ADI Required Write
    // 42 81 51 ; ADI Required Write
    // 42 82 68 ; ADI Required Write
    decoder.select_submap(DEC_SUBMAP_0x80);
    uint8_t dec_req1[] = { 0x9c, 0x00 };
    I2c_HW.write_multi(decoder.address, dec_req1, dec_req1 + 2);
    uint8_t dec_req2[] = { 0x9c, 0xff };
    I2c_HW.write_multi(decoder.address, dec_req2, dec_req2 + 2);
    decoder.select_submap(DEC_SUBMAP_USER);
    uint8_t dec_req3[] = { 0x80, 0x51 };
    I2c_HW.write_multi(decoder.address, dec_req3, dec_req3 + 2);
    uint8_t dec_req4[] = { 0x81, 0x51 };
    I2c_HW.write_multi(decoder.address, dec_req4, dec_req4 + 2);
    uint8_t dec_req5[] = { 0x82, 0x68 };
    I2c_HW.write_multi(decoder.address, dec_req5, dec_req5 + 2);
}

static void setup_cvbs(bool pedestal, bool antialias = false)
{
    // Software reset decoder and encoder
    decoder.set_power_management(false, true);
    uint8_t enc_reset_seq[] = { 0x17, 0x02 };
    I2c_HW.write_multi(encoder.address, enc_reset_seq, enc_reset_seq + sizeof(enc_reset_seq));
    _delay_ms(10);

    // Decoder setup

    // Exit powerdown
    decoder.set_power_management(false, false);

    // AFE IBIAS (undocumented register, used in recommended scripts)
    uint8_t dec_afe_ibias[] = { 0x52, 0xcd };
    I2c_HW.write_multi(decoder.address, dec_afe_ibias, dec_afe_ibias + 2);

    // Use CVBS input on A_in1
    decoder.select_input(INSEL_CVBS_Ain1);

    setup_ad_black_magic();

    // Shaping filter control 1, AD recommendation for CVBS
    uint8_t shafc1[] = { 0x17, 0x41 };
    I2c_HW.write_multi(decoder.address, shafc1, shafc1 + 2);

    // Autodetect SD video mode
    if (pedestal)
        decoder.select_autodetection(AD_PALBGHID_NTSCM_SECAM);
    else
        decoder.select_autodetection(AD_PALBGHID_NTSCJ_SECAM);

    // Output control
    // Enable output drivers, enable VBI
    decoder.set_output_control(false, true);

    // Extended output control
    // Output full range, enable SFL, blank chroma during VBI
    decoder.set_ext_output_control(true, true, true, false, false);

    // A write to a supposedly read-only register, recommended by AD scripts.
    {
        uint8_t dec_req1[] = { 0x13, 0x00 };
        I2c_HW.write_multi(decoder.address, dec_req1, dec_req1 + 2);
    }

    // Analog clamp control
    // 100% color bars
    uint8_t ana_clampc[] = { 0x14, 0x11 };
    I2c_HW.write_multi(decoder.address, ana_clampc, ana_clampc + 2);

    // Digital clamp control
    // Digital clamp on, time constant adaptive
    uint8_t dig_clampc[] = { 0x15, 0x60 };
    I2c_HW.write_multi(decoder.address, dig_clampc, dig_clampc + 2);

    // Shaping filter control 2
    // Select best filter automagically
    uint8_t shafc2[] = { 0x18, 0x13 };
    I2c_HW.write_multi(decoder.address, shafc2, shafc2 + 2);

#if 0
    // Comb filter control
    // PAL: wide bandwidth, NTSC: medium-low bandwidth (01)
    uint8_t combfc[] = { 0x19, 0xf6 };
    I2c_HW.write_multi(decoder.address, combfc, combfc + 2);
#endif

    // Analog Devices control 2
    // LLC pin active
    uint8_t adc2[] = { 0x1d, 0x40 };
    I2c_HW.write_multi(decoder.address, adc2, adc2 + 2);

    // VS/FIELD Control 1
    // EAV/SAV codes generated for Analog Devices encoder
    uint8_t vs_fieldc[] = { 0x31, 0x02 };
    I2c_HW.write_multi(decoder.address, vs_fieldc, vs_fieldc + sizeof(vs_fieldc));

    // CTI DNR control
    // Disable CTI and CTI alpha blender, enable DNR
    decoder.set_cti_dnr_control(false, false, AB_SMOOTHEST, true);

    // Output sync select 2
    // Output SFL on the VS/FIELD/SFL pin
    uint8_t out_sync_sel2[] = { 0x6b, 0x14 };
    I2c_HW.write_multi(decoder.address, out_sync_sel2, out_sync_sel2 + 2);

    // Antialiasing Filter Control 1
    decoder.set_aa_filters(!antialias, false, false, false, false);

#if 0
    // Drive strength of digital outputs
    // Low drive strength for all
    uint8_t dr_str[] = { 0xf4, 0x00 };
    I2c_HW.write_multi(decoder.address, dr_str, dr_str + 2);
#endif

    // Encoder setup
    setup_encoder();
}

void setup_svideo(bool pedestal, bool antialias = false)
{
    // Software reset decoder and encoder
    decoder.set_power_management(false, true);
    uint8_t enc_reset_seq[] = { 0x17, 0x02 };
    I2c_HW.write_multi(encoder.address, enc_reset_seq, enc_reset_seq + sizeof(enc_reset_seq));
    _delay_ms(10);

    // Decoder setup

    // Exit powerdown
    decoder.set_power_management(false, false);

    // AFE IBIAS (undocumented register, used in recommended scripts)
    uint8_t dec_afe_ibias[] = { 0x53, 0xce };
    I2c_HW.write_multi(decoder.address, dec_afe_ibias, dec_afe_ibias + 2);

    // Use YC input on A_in3 and A_in4
    decoder.select_input(INSEL_YC_Ain3_4);

    setup_ad_black_magic();

    // Autodetect SD video mode
    if (pedestal)
        decoder.select_autodetection(AD_PALBGHID_NTSCM_SECAM);
    else
        decoder.select_autodetection(AD_PALBGHID_NTSCJ_SECAM);

    // Output control
    // Enable output drivers, enable VBI
    decoder.set_output_control(false, true);

    // Extended output control
    // Output full range, enable SFL, blank chroma during VBI
    decoder.set_ext_output_control(true, true, true, false, false);

    // A write to a supposedly read-only register, recommended by AD scripts.
    {
        uint8_t dec_req1[] = { 0x13, 0x00 };
        I2c_HW.write_multi(decoder.address, dec_req1, dec_req1 + 2);
    }

    // Analog clamp control
    // 100% color bars
    uint8_t ana_clampc[] = { 0x14, 0x11 };
    I2c_HW.write_multi(decoder.address, ana_clampc, ana_clampc + 2);

    // Digital clamp control
    // Digital clamp on, time constant adaptive
    uint8_t dig_clampc[] = { 0x15, 0x60 };
    I2c_HW.write_multi(decoder.address, dig_clampc, dig_clampc + 2);

    // Shaping filter control 2
    // Select best filter automagically
    uint8_t shafc2[] = { 0x18, 0x13 };
    I2c_HW.write_multi(decoder.address, shafc2, shafc2 + 2);

#if 0
    // Comb filter control
    // PAL: wide bandwidth, NTSC: medium-low bandwidth (01)
    uint8_t combfc[] = { 0x19, 0xf6 };
    I2c_HW.write_multi(decoder.address, combfc, combfc + 2);
#endif

    // Analog Devices control 2
    // LLC pin active
    uint8_t adc2[] = { 0x1d, 0x40 };
    I2c_HW.write_multi(decoder.address, adc2, adc2 + 2);

    // VS/FIELD Control 1
    // EAV/SAV codes generated for Analog Devices encoder
    uint8_t vs_fieldc[] = { 0x31, 0x02 };
    I2c_HW.write_multi(decoder.address, vs_fieldc, vs_fieldc + sizeof(vs_fieldc));

    // CTI DNR control
    // Disable CTI and CTI alpha blender, enable DNR
    decoder.set_cti_dnr_control(false, false, AB_SMOOTHEST, true);

    // Output sync select 2
    // Output SFL on the VS/FIELD/SFL pin
    uint8_t out_sync_sel2[] = { 0x6b, 0x14 };
    I2c_HW.write_multi(decoder.address, out_sync_sel2, out_sync_sel2 + 2);

    // Antialiasing Filter Control 1
    decoder.set_aa_filters(!antialias, false, false, false, false);

#if 0
    // Drive strength of digital outputs
    // Low drive strength for all
    uint8_t dr_str[] = { 0xf4, 0x00 };
    I2c_HW.write_multi(decoder.address, dr_str, dr_str + 2);
#endif

    // Encoder setup
    setup_encoder();
}

int main(void)
{
    _delay_ms(100);
    cli();

    // Setup reading the "input change" and "option" switches
    input_change.mode = INPUT_PULLUP;
    option.mode = INPUT_PULLUP;
    setup_timer0();

    // Using external 2kohm pullups for the I2C bus
    sda.mode = INPUT;
    scl.mode = INPUT;

    I2c_HW.setup();

#if DEBUG || CALIBRATE
    serial.setup(38400, DATA_EIGHT, STOP_ONE, PARITY_DISABLED);
#endif

    sei();

    _delay_ms(6);
    decoder.reset = true;
    decoder.pwrdwn = true;
    encoder.reset = true;
    _delay_ms(10);

#if CALIBRATE
    const auto old_osccal = OSCCAL;
    const auto osccal_min = (old_osccal < 20) ? 0 : (old_osccal - 20);
    const auto osccal_max = (old_osccal > 0xff - 20) ? 0xff : (old_osccal + 20);
    for (auto i = osccal_min; i != osccal_max; ++i) {
        OSCCAL = i;
        _delay_ms(10);
        serial << _T("OSCCAL = 0x") << ashex(i) << _T(" (old: 0x")
            << ashex(old_osccal) << _T(") The quick brown fox jumps over the lazy dog. åäö, ÅÄÖ\r\n");
        OSCCAL = old_osccal;
        for (size_t j = 0; j < 10; ++j)
            serial << _T("\r\n");
    }
    OSCCAL = old_osccal;

    return 0;
#endif

#if DEBUG
    serial << _T("converter starting...\r\n");
#endif

    setup_cvbs(false);
    curr_input = CVBS;

#if DEBUG
    // A simple loop to read and print out status changes in the decoder
    uint8_t dec_status1 = 0x00;
    uint8_t dec_status2 = 0x00;
    uint8_t dec_status3 = 0x00;
#endif
    while (1) {
        if (read_input_change()) {
            switch(curr_input) {
            case CVBS:
                setup_cvbs(true);
                curr_input = CVBS_PEDESTAL;
                break;
            case CVBS_PEDESTAL:
                setup_svideo(false);
                curr_input = SVIDEO;
                break;
            case SVIDEO:
                setup_svideo(true);
                curr_input = SVIDEO_PEDESTAL;
                break;
            case SVIDEO_PEDESTAL:
                setup_cvbs(false);
                curr_input = CVBS;
                break;
            }
        }

#if DEBUG
        I2c_HW.write(decoder.address, 0x10, true, false);
        uint8_t new_status1 = I2c_HW.read(decoder.address);
        I2c_HW.write(decoder.address, 0x12, true, false);
        uint8_t new_status2 = I2c_HW.read(decoder.address);
        I2c_HW.write(decoder.address, 0x13, true, false);
        uint8_t new_status3 = I2c_HW.read(decoder.address);

        if (new_status1 != dec_status1) {
            serial << _T("Status 1 changed:\r\n");
            serial << _T("In lock: ") << asdec(new_status1 & 0x01) << _T("\r\n");
            serial << _T("Lost lock: ") << asdec(!!(new_status1 & 0x02)) << _T("\r\n");
            serial << _T("fSC lock: ") << asdec(!!(new_status1 & 0x04)) << _T("\r\n");
            serial << _T("Follow PW: ") << asdec(!!(new_status1 & 0x08)) << _T("\r\n");
            serial << _T("Video standard: ");
            switch ((new_status1 >> 4) & 0x07) {
            case 0x00:
                serial << _T("NTSC M/J\r\n");
                break;
            case 0x01:
                serial << _T("NTSC 4.43\r\n");
                break;
            case 0x02:
                serial << _T("PAL M\r\n");
                break;
            case 0x03:
                serial << _T("PAL 60\r\n");
                break;
            case 0x04:
                serial << _T("PAL B/G/H/I/D\r\n");
                break;
            case 0x05:
                serial << _T("SECAM\r\n");
                break;
            case 0x06:
                serial << _T("PAL Combination N\r\n");
                break;
            case 0x07:
                serial << _T("SECAM 525\r\n");
                break;
            }
            serial << _T("Color kill: ") << asdec(!!(new_status1 & 0x80)) << _T("\r\n");
            serial << _T("\r\n");
        }
        dec_status1 = new_status1;

        if (new_status2 != dec_status2) {
            serial << _T("Status 2 changed:\r\n");
            serial << _T("Line length nonstandard: ") << asdec(!!(new_status2 & 0x10)) << _T("\r\n");
            serial << _T("fSC nonstandard: ") << asdec(!!(new_status2 & 0x20)) << _T("\r\n");
            serial << _T("\r\n");
        }
        dec_status2 = new_status2;

        if (new_status3 != dec_status3) {
            serial << _T("Status 3 changed:\r\n");
            serial << _T("Horizontal lock: ") << asdec(new_status3 & 0x01) << _T("\r\n");
            serial << _T("Frequency: ") << ((new_status3 & 0x04) ? _T("50") : _T("60")) << _T("\r\n");
            serial << _T("Freerun active: ") << asdec(!!(new_status3 & 0x10)) << _T("\r\n");
            serial << _T("Field length standard: ") << asdec(!!(new_status3 & 0x20)) << _T("\r\n");
            serial << _T("Interlaced: ") << asdec(!!(new_status3 & 0x40)) << _T("\r\n");
            serial << _T("PAL SW lock: ") << asdec(!!(new_status3 & 0x80)) << _T("\r\n");
            serial << _T("\r\n");
        }
        dec_status3 = new_status3;
#endif

        _delay_ms(100);
    }

    I2c_HW.deinit();

    return 0;
}
