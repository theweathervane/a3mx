/* The characters' move inputs, looked at every frame a fighter is free to act: a chain of checks
   in the order the game prefers them, each with the place the input's progress is kept (an offset
   in the fighter) and the input's number; the first check that passes and whose move then starts
   ends the chain. Eighteen characters' chains are plain lists. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08867708(Cpu *c); void f_08868488(Cpu *c); void f_08868640(Cpu *c); void f_08868A34(Cpu *c);
void f_08869008(Cpu *c); void f_088697C0(Cpu *c); void f_088699FC(Cpu *c); void f_08869C88(Cpu *c);
void f_0886A1E4(Cpu *c); void f_0886A264(Cpu *c); void f_0886A2D8(Cpu *c); void f_088F2B94(Cpu *c);
void f_088F2C48(Cpu *c); void f_088F2CEC(Cpu *c); void f_088F2D9C(Cpu *c); void f_088F2E14(Cpu *c);
void f_088F2F60(Cpu *c); void f_088F2FF0(Cpu *c); void f_088F3060(Cpu *c); void f_088F3104(Cpu *c);
void f_088F3194(Cpu *c); void f_088F3228(Cpu *c); void f_088F7404(Cpu *c); void f_088F74B8(Cpu *c);
void f_088F7530(Cpu *c); void f_088F75E0(Cpu *c); void f_088F7684(Cpu *c); void f_088F7710(Cpu *c);
void f_088F77C4(Cpu *c); void f_088F7878(Cpu *c); void f_088F7950(Cpu *c); void f_088F79C0(Cpu *c);
void f_088F7A54(Cpu *c); void f_088F7B94(Cpu *c); void f_088F7C30(Cpu *c); void f_088F7CD0(Cpu *c);
void f_088F7DC4(Cpu *c); void f_088FD370(Cpu *c); void f_088FD448(Cpu *c); void f_088FD4D4(Cpu *c);
void f_088FD578(Cpu *c); void f_088FD614(Cpu *c); void f_088FD6C4(Cpu *c); void f_088FD770(Cpu *c);
void f_088FD81C(Cpu *c); void f_088FD978(Cpu *c); void f_08901B0C(Cpu *c); void f_08901B7C(Cpu *c);
void f_08901BFC(Cpu *c); void f_08901CC4(Cpu *c); void f_08901D44(Cpu *c); void f_08901DF4(Cpu *c);
void f_08901EB0(Cpu *c); void f_08901F30(Cpu *c); void f_08901FBC(Cpu *c); void f_0890203C(Cpu *c);
void f_089020E8(Cpu *c); void f_08902260(Cpu *c); void f_089062C8(Cpu *c); void f_08906350(Cpu *c);
void f_089063D0(Cpu *c); void f_0890645C(Cpu *c); void f_089064E0(Cpu *c); void f_08906580(Cpu *c);
void f_08906694(Cpu *c); void f_08906770(Cpu *c); void f_08909820(Cpu *c); void f_08909888(Cpu *c);
void f_08909924(Cpu *c); void f_08909998(Cpu *c); void f_08909A20(Cpu *c); void f_08909B68(Cpu *c);
void f_08909BDC(Cpu *c); void f_08909C50(Cpu *c); void f_08909CBC(Cpu *c); void f_08909D28(Cpu *c);
void f_08909DD4(Cpu *c); void f_08909EB0(Cpu *c); void f_0890FD3C(Cpu *c); void f_0890FDF0(Cpu *c);
void f_0890FE90(Cpu *c); void f_0890FF18(Cpu *c); void f_0890FF9C(Cpu *c); void f_0891003C(Cpu *c);
void f_089100E8(Cpu *c); void f_08910154(Cpu *c); void f_089101D4(Cpu *c); void f_089103A8(Cpu *c);
void f_08918CCC(Cpu *c); void f_08918DA0(Cpu *c); void f_08918E20(Cpu *c); void f_08918EA4(Cpu *c);
void f_08918F28(Cpu *c); void f_08919068(Cpu *c); void f_08919110(Cpu *c); void f_089191BC(Cpu *c);
void f_08919264(Cpu *c); void f_0891931C(Cpu *c); void f_0892A59C(Cpu *c); void f_0892A634(Cpu *c);
void f_0892A6D8(Cpu *c); void f_0892A748(Cpu *c); void f_0892A7BC(Cpu *c); void f_0892A878(Cpu *c);
void f_0892A934(Cpu *c); void f_0892A9D4(Cpu *c); void f_0892AA80(Cpu *c); void f_0892AB8C(Cpu *c);
void f_0892AC60(Cpu *c); void f_0892AD04(Cpu *c); void f_0892AD84(Cpu *c); void f_0892FE1C(Cpu *c);
void f_0892FEC0(Cpu *c); void f_0892FF84(Cpu *c); void f_089300C8(Cpu *c); void f_0893014C(Cpu *c);
void f_08930204(Cpu *c); void f_089302B0(Cpu *c); void f_08930350(Cpu *c); void f_089303F0(Cpu *c);
void f_0893048C(Cpu *c); void f_0895C4FC(Cpu *c); void f_0895C594(Cpu *c); void f_0895C654(Cpu *c);
void f_0895C6D8(Cpu *c); void f_0895C7A8(Cpu *c); void f_0895C82C(Cpu *c); void f_0895C8BC(Cpu *c);
void f_0895C940(Cpu *c); void f_0895CA08(Cpu *c); void f_0895CAA8(Cpu *c); void f_0895CB48(Cpu *c);
void f_0895CBD4(Cpu *c); void f_089672EC(Cpu *c); void f_0896739C(Cpu *c); void f_08967438(Cpu *c);
void f_089674D8(Cpu *c); void f_0896756C(Cpu *c); void f_089676BC(Cpu *c); void f_08967740(Cpu *c);
void f_089677C4(Cpu *c); void f_08967848(Cpu *c); void f_089678D0(Cpu *c); void f_089734F0(Cpu *c);
void f_089735A0(Cpu *c); void f_08973620(Cpu *c); void f_089736A8(Cpu *c); void f_089737F0(Cpu *c);
void f_08973880(Cpu *c); void f_08973910(Cpu *c); void f_08973984(Cpu *c); void f_089739F8(Cpu *c);
void f_08973A78(Cpu *c); void f_08973AF8(Cpu *c); void f_0897BCC0(Cpu *c); void f_0897BD74(Cpu *c);
void f_0897BE54(Cpu *c); void f_0897BEDC(Cpu *c); void f_0897BFE8(Cpu *c); void f_0897C088(Cpu *c);
void f_0897C11C(Cpu *c); void f_0897C1DC(Cpu *c); void f_0897C264(Cpu *c); void f_0897C2EC(Cpu *c);
void f_0898A040(Cpu *c); void f_0898A0D0(Cpu *c); void f_0898A164(Cpu *c); void f_0898A1F8(Cpu *c);
void f_0898A284(Cpu *c); void f_0898A344(Cpu *c); void f_0898A3CC(Cpu *c); void f_0898A454(Cpu *c);
void f_0898A4DC(Cpu *c); void f_0898A568(Cpu *c); void f_0898A5EC(Cpu *c); void f_0898A69C(Cpu *c);
void f_0898A7EC(Cpu *c); void f_0899B058(Cpu *c); void f_0899B13C(Cpu *c); void f_0899B1D0(Cpu *c);
void f_0899B394(Cpu *c); void f_0899B424(Cpu *c); void f_0899B4CC(Cpu *c); void f_0899B578(Cpu *c);
void f_089A30F0(Cpu *c); void f_089A31A0(Cpu *c); void f_089A3234(Cpu *c); void f_089A32D8(Cpu *c);
void f_089A3358(Cpu *c); void f_089A33D8(Cpu *c); void f_089A3464(Cpu *c); void f_089A34FC(Cpu *c);
void f_089A3588(Cpu *c); void f_089A3614(Cpu *c); void f_089A5E18(Cpu *c); void f_089A5EA0(Cpu *c);
void f_089A5F54(Cpu *c); void f_089A60A4(Cpu *c); void f_089A6138(Cpu *c); void f_089A61C4(Cpu *c);
void f_089A624C(Cpu *c); void f_089A62E0(Cpu *c); void f_089A6374(Cpu *c);

#define NONE 0xFFFFFFFFu

/* One step: `check` (fighter, fighter + off, code), where NONE leaves that argument as it was, and
   the move's `start` (fighter) when it passes. */
typedef struct {
    void (*check)(Cpu *);
    uint32_t off, code, back_check;
    void (*start)(Cpu *);
    uint32_t back_start;
} Check;

/* (fighter): the steps in order; a start that returns nonzero ends them. The last start's answer
   is not looked at. A 0x20 frame keeping s0. */
static void run_checks(Cpu *c, const Check *t, int n) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    for (int i = 0; i < n; i++) {
        if (t[i].off != NONE) ARG(1) = o + t[i].off;
        ARG(0) = o;
        if (t[i].code != NONE) ARG(2) = t[i].code;
        c->r[31] = t[i].back_check;
        t[i].check(c);
        if (c->r[2] == 0) continue;
        ARG(0) = o;
        c->r[31] = t[i].back_start;
        t[i].start(c);
        if (c->r[2] != 0) break;
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

SFA_FUNC(0899AF44, move_inputs_9af44) {
    static const Check t[7] = {
        { f_0886A1E4, NONE, NONE, 0x0899AF58u, f_0899B058, 0x0899AF68u },
        { f_08869008, 0x360, 0x35, 0x0899AF80u, f_0899B4CC, 0x0899AF90u },
        { f_08869008, 0x368, 0x31, 0x0899AFA8u, f_0899B578, 0x0899AFB8u },
        { f_08869008, 0x350, 0x2B, 0x0899AFD0u, f_0899B394, 0x0899AFE0u },
        { f_08869008, 0x358, 0x2C, 0x0899AFF8u, f_0899B424, 0x0899B008u },
        { f_0886A264, NONE, NONE, 0x0899B018u, f_0899B1D0, 0x0899B028u },
        { f_0886A2D8, NONE, NONE, 0x0899B038u, f_0899B13C, 0x0899B048u },
    };
    run_checks(c, t, 7);
}
SFA_FUNC(0890618C, move_inputs_0618c) {
    static const Check t[8] = {
        { f_0886A1E4, NONE, NONE, 0x089061A0u, f_08906770, 0x089061B0u },
        { f_08867708, 0x370, 0x17, 0x089061C8u, f_08906580, 0x089061D8u },
        { f_08867708, 0x368, 0x18, 0x089061F0u, f_089064E0, 0x08906200u },
        { f_08867708, 0x358, 0x6, 0x08906218u, f_08906350, 0x08906228u },
        { f_08867708, 0x350, 0x8, 0x08906240u, f_089063D0, 0x08906250u },
        { f_08867708, 0x360, 0x14, 0x08906268u, f_0890645C, 0x08906278u },
        { f_0886A264, NONE, NONE, 0x08906288u, f_08906694, 0x08906298u },
        { f_0886A2D8, NONE, NONE, 0x089062A8u, f_089062C8, 0x089062B8u },
    };
    run_checks(c, t, 8);
}
SFA_FUNC(089A5CBC, move_inputs_a5cbc) {
    static const Check t[9] = {
        { f_0886A1E4, NONE, NONE, 0x089A5CD0u, f_089A5EA0, 0x089A5CE0u },
        { f_08867708, 0x378, 0x17, 0x089A5CF8u, f_089A62E0, 0x089A5D08u },
        { f_08868A34, 0x370, 0x27, 0x089A5D20u, f_089A624C, 0x089A5D30u },
        { f_08868A34, 0x368, 0x26, 0x089A5D48u, f_089A61C4, 0x089A5D58u },
        { f_08867708, 0x350, 0x5, 0x089A5D70u, f_089A5E18, 0x089A5D80u },
        { f_08869C88, NONE, NONE, 0x089A5D90u, f_089A6138, 0x089A5DA0u },
        { f_08867708, 0x360, 0x7, 0x089A5DB8u, f_089A60A4, 0x089A5DC8u },
        { f_0886A264, NONE, NONE, 0x089A5DD8u, f_089A5F54, 0x089A5DE8u },
        { f_0886A2D8, NONE, NONE, 0x089A5DF8u, f_089A6374, 0x089A5E08u },
    };
    run_checks(c, t, 9);
}
SFA_FUNC(088FD20C, move_inputs_fd20c) {
    static const Check t[9] = {
        { f_0886A1E4, NONE, NONE, 0x088FD220u, f_088FD370, 0x088FD230u },
        { f_08869008, 0x370, 0x31, 0x088FD248u, f_088FD770, 0x088FD258u },
        { f_08869008, 0x368, 0x35, 0x088FD270u, f_088FD6C4, 0x088FD280u },
        { f_08869008, 0x360, 0x32, 0x088FD298u, f_088FD614, 0x088FD2A8u },
        { f_088697C0, 0x380, 0x0, 0x088FD2C0u, f_088FD578, 0x088FD2D0u },
        { f_0886A264, NONE, NONE, 0x088FD2E0u, f_088FD81C, 0x088FD2F0u },
        { f_08869008, 0x358, 0x2C, 0x088FD308u, f_088FD4D4, 0x088FD318u },
        { f_08869008, 0x350, 0x2B, 0x088FD330u, f_088FD448, 0x088FD340u },
        { f_0886A2D8, NONE, NONE, 0x088FD350u, f_088FD978, 0x088FD360u },
    };
    run_checks(c, t, 9);
}
SFA_FUNC(0890FBB0, move_inputs_0fbb0) {
    static const Check t[10] = {
        { f_0886A1E4, NONE, NONE, 0x0890FBC4u, f_0890FD3C, 0x0890FBD4u },
        { f_08867708, 0x350, 0x1E, 0x0890FBECu, f_0890FDF0, 0x0890FBFCu },
        { f_08867708, 0x358, 0x4, 0x0890FC14u, f_0890FF18, 0x0890FC24u },
        { f_08867708, 0x360, 0x17, 0x0890FC3Cu, f_0891003C, 0x0890FC4Cu },
        { f_08867708, 0x368, 0x18, 0x0890FC64u, f_0890FF9C, 0x0890FC74u },
        { f_08867708, 0x370, 0x1, 0x0890FC8Cu, f_089100E8, 0x0890FC9Cu },
        { f_08867708, 0x378, 0x2, 0x0890FCB4u, f_08910154, 0x0890FCC4u },
        { f_08867708, 0x380, 0x3, 0x0890FCDCu, f_089101D4, 0x0890FCECu },
        { f_0886A264, NONE, NONE, 0x0890FCFCu, f_089103A8, 0x0890FD0Cu },
        { f_0886A2D8, NONE, NONE, 0x0890FD1Cu, f_0890FE90, 0x0890FD2Cu },
    };
    run_checks(c, t, 10);
}
SFA_FUNC(08918B40, move_inputs_18b40) {
    static const Check t[10] = {
        { f_0886A1E4, NONE, NONE, 0x08918B54u, f_0891931C, 0x08918B64u },
        { f_08867708, 0x378, 0x18, 0x08918B7Cu, f_089191BC, 0x08918B8Cu },
        { f_08867708, 0x368, 0x17, 0x08918BA4u, f_08919068, 0x08918BB4u },
        { f_08867708, 0x370, 0x1D, 0x08918BCCu, f_08919110, 0x08918BDCu },
        { f_08867708, 0x358, 0x11, 0x08918BF4u, f_08918CCC, 0x08918C04u },
        { f_08867708, 0x350, 0x3, 0x08918C1Cu, f_08918DA0, 0x08918C2Cu },
        { f_08867708, 0x360, 0x5, 0x08918C44u, f_08918E20, 0x08918C54u },
        { f_08867708, 0x380, 0x2, 0x08918C6Cu, f_08918EA4, 0x08918C7Cu },
        { f_0886A264, NONE, NONE, 0x08918C8Cu, f_08918F28, 0x08918C9Cu },
        { f_0886A2D8, NONE, NONE, 0x08918CACu, f_08919264, 0x08918CBCu },
    };
    run_checks(c, t, 10);
}
SFA_FUNC(0892FC90, move_inputs_2fc90) {
    static const Check t[10] = {
        { f_0886A1E4, NONE, NONE, 0x0892FCA4u, f_0892FEC0, 0x0892FCB4u },
        { f_08867708, 0x368, 0x1C, 0x0892FCCCu, f_08930204, 0x0892FCDCu },
        { f_08867708, 0x370, 0x18, 0x0892FCF4u, f_089302B0, 0x0892FD04u },
        { f_08867708, 0x378, 0x1B, 0x0892FD1Cu, f_08930350, 0x0892FD2Cu },
        { f_08867708, 0x358, 0x5, 0x0892FD44u, f_089300C8, 0x0892FD54u },
        { f_08867708, 0x380, 0x6, 0x0892FD6Cu, f_0893048C, 0x0892FD7Cu },
        { f_08867708, 0x350, 0x1, 0x0892FD94u, f_0892FE1C, 0x0892FDA4u },
        { f_08867708, 0x360, 0x4, 0x0892FDBCu, f_0893014C, 0x0892FDCCu },
        { f_0886A264, NONE, NONE, 0x0892FDDCu, f_0892FF84, 0x0892FDECu },
        { f_0886A2D8, NONE, NONE, 0x0892FDFCu, f_089303F0, 0x0892FE0Cu },
    };
    run_checks(c, t, 10);
}
SFA_FUNC(08967160, move_inputs_67160) {
    static const Check t[10] = {
        { f_0886A1E4, NONE, NONE, 0x08967174u, f_089672EC, 0x08967184u },
        { f_08869008, 0x350, 0x31, 0x0896719Cu, f_0896739C, 0x089671ACu },
        { f_08868A34, 0x358, 0x27, 0x089671C4u, f_089674D8, 0x089671D4u },
        { f_08868A34, 0x360, 0x26, 0x089671ECu, f_089678D0, 0x089671FCu },
        { f_08869008, 0x368, 0x32, 0x08967214u, f_08967438, 0x08967224u },
        { f_0886A264, NONE, NONE, 0x08967234u, f_0896756C, 0x08967244u },
        { f_08869008, 0x370, 0x2C, 0x0896725Cu, f_089676BC, 0x0896726Cu },
        { f_08869008, 0x378, 0x2B, 0x08967284u, f_08967740, 0x08967294u },
        { f_088699FC, 0x380, 0x3E, 0x089672ACu, f_089677C4, 0x089672BCu },
        { f_0886A2D8, NONE, NONE, 0x089672CCu, f_08967848, 0x089672DCu },
    };
    run_checks(c, t, 10);
}
SFA_FUNC(0897BB34, move_inputs_7bb34) {
    static const Check t[10] = {
        { f_0886A1E4, NONE, NONE, 0x0897BB48u, f_0897BCC0, 0x0897BB58u },
        { f_08867708, 0x370, 0x1C, 0x0897BB70u, f_0897BEDC, 0x0897BB80u },
        { f_08867708, 0x378, 0x18, 0x0897BB98u, f_0897BFE8, 0x0897BBA8u },
        { f_08867708, 0x358, 0xA, 0x0897BBC0u, f_0897C088, 0x0897BBD0u },
        { f_08867708, 0x368, 0x1, 0x0897BBE8u, f_0897C11C, 0x0897BBF8u },
        { f_08867708, 0x350, 0x3, 0x0897BC10u, f_0897C2EC, 0x0897BC20u },
        { f_08867708, 0x360, 0x2, 0x0897BC38u, f_0897C1DC, 0x0897BC48u },
        { f_0886A264, NONE, NONE, 0x0897BC58u, f_0897BD74, 0x0897BC68u },
        { f_08868640, 0x380, 0x22, 0x0897BC80u, f_0897BE54, 0x0897BC90u },
        { f_0886A2D8, NONE, NONE, 0x0897BCA0u, f_0897C264, 0x0897BCB0u },
    };
    run_checks(c, t, 10);
}
SFA_FUNC(089A2F64, move_inputs_a2f64) {
    static const Check t[10] = {
        { f_0886A1E4, NONE, NONE, 0x089A2F78u, f_089A30F0, 0x089A2F88u },
        { f_08869008, 0x350, 0x34, 0x089A2FA0u, f_089A3588, 0x089A2FB0u },
        { f_08869008, 0x358, 0x35, 0x089A2FC8u, f_089A34FC, 0x089A2FD8u },
        { f_08869008, 0x360, 0x32, 0x089A2FF0u, f_089A3464, 0x089A3000u },
        { f_08869008, 0x368, 0x2C, 0x089A3018u, f_089A3234, 0x089A3028u },
        { f_08869008, 0x370, 0x2D, 0x089A3040u, f_089A32D8, 0x089A3050u },
        { f_08869008, 0x378, 0x2A, 0x089A3068u, f_089A3358, 0x089A3078u },
        { f_08869008, 0x380, 0x2B, 0x089A3090u, f_089A33D8, 0x089A30A0u },
        { f_0886A264, NONE, NONE, 0x089A30B0u, f_089A3614, 0x089A30C0u },
        { f_0886A2D8, NONE, NONE, 0x089A30D0u, f_089A31A0, 0x089A30E0u },
    };
    run_checks(c, t, 10);
}
SFA_FUNC(088F29E0, move_inputs_f29e0) {
    static const Check t[11] = {
        { f_0886A1E4, NONE, NONE, 0x088F29F4u, f_088F2B94, 0x088F2A04u },
        { f_08867708, 0x370, 0x17, 0x088F2A1Cu, f_088F3104, 0x088F2A2Cu },
        { f_08867708, 0x368, 0x18, 0x088F2A44u, f_088F2F60, 0x088F2A54u },
        { f_08867708, 0x388, 0x1B, 0x088F2A6Cu, f_088F3194, 0x088F2A7Cu },
        { f_08867708, 0x358, 0x5, 0x088F2A94u, f_088F2FF0, 0x088F2AA4u },
        { f_08867708, 0x350, 0x1, 0x088F2ABCu, f_088F3060, 0x088F2ACCu },
        { f_08867708, 0x378, 0x3, 0x088F2AE4u, f_088F2CEC, 0x088F2AF4u },
        { f_08867708, 0x360, 0x4, 0x088F2B0Cu, f_088F3228, 0x088F2B1Cu },
        { f_08868640, 0x380, 0x22, 0x088F2B34u, f_088F2D9C, 0x088F2B44u },
        { f_0886A264, NONE, NONE, 0x088F2B54u, f_088F2E14, 0x088F2B64u },
        { f_0886A2D8, NONE, NONE, 0x088F2B74u, f_088F2C48, 0x088F2B84u },
    };
    run_checks(c, t, 11);
}
SFA_FUNC(0897333C, move_inputs_7333c) {
    static const Check t[11] = {
        { f_0886A1E4, NONE, NONE, 0x08973350u, f_089734F0, 0x08973360u },
        { f_08868A34, 0x388, 0x27, 0x08973378u, f_08973A78, 0x08973388u },
        { f_08868A34, 0x378, 0x26, 0x089733A0u, f_08973984, 0x089733B0u },
        { f_08868A34, 0x360, 0x28, 0x089733C8u, f_08973AF8, 0x089733D8u },
        { f_08867708, 0x380, 0x17, 0x089733F0u, f_089739F8, 0x08973400u },
        { f_08867708, 0x350, 0x18, 0x08973418u, f_089737F0, 0x08973428u },
        { f_08867708, 0x358, 0x3, 0x08973440u, f_089735A0, 0x08973450u },
        { f_08867708, 0x370, 0x4, 0x08973468u, f_08973910, 0x08973478u },
        { f_08867708, 0x368, 0x14, 0x08973490u, f_08973880, 0x089734A0u },
        { f_0886A264, NONE, NONE, 0x089734B0u, f_089736A8, 0x089734C0u },
        { f_0886A2D8, NONE, NONE, 0x089734D0u, f_08973620, 0x089734E0u },
    };
    run_checks(c, t, 11);
}
SFA_FUNC(08901930, move_inputs_01930) {
    static const Check t[12] = {
        { f_0886A1E4, NONE, NONE, 0x08901944u, f_08902260, 0x08901954u },
        { f_08867708, 0x390, 0x1C, 0x0890196Cu, f_08901FBC, 0x0890197Cu },
        { f_08869008, 0x388, 0x35, 0x08901994u, f_08901F30, 0x089019A4u },
        { f_08869008, 0x380, 0x32, 0x089019BCu, f_08901EB0, 0x089019CCu },
        { f_08869008, 0x360, 0x2C, 0x089019E4u, f_08901CC4, 0x089019F4u },
        { f_08869008, 0x378, 0x2D, 0x08901A0Cu, f_08901BFC, 0x08901A1Cu },
        { f_08869008, 0x368, 0x2B, 0x08901A34u, f_08901DF4, 0x08901A44u },
        { f_08867708, 0x358, 0xD, 0x08901A5Cu, f_08901D44, 0x08901A6Cu },
        { f_08867708, 0x370, 0x14, 0x08901A84u, f_08901B7C, 0x08901A94u },
        { f_088699FC, 0x350, 0x3F, 0x08901AACu, f_08901B0C, 0x08901ABCu },
        { f_0886A264, NONE, NONE, 0x08901ACCu, f_089020E8, 0x08901ADCu },
        { f_0886A2D8, NONE, NONE, 0x08901AECu, f_0890203C, 0x08901AFCu },
    };
    run_checks(c, t, 12);
}
SFA_FUNC(08909644, move_inputs_09644) {
    static const Check t[12] = {
        { f_0886A1E4, NONE, NONE, 0x08909658u, f_08909DD4, 0x08909668u },
        { f_08868A34, 0x360, 0x27, 0x08909680u, f_08909998, 0x08909690u },
        { f_08867708, 0x368, 0x1C, 0x089096A8u, f_08909888, 0x089096B8u },
        { f_08868A34, 0x350, 0x26, 0x089096D0u, f_08909820, 0x089096E0u },
        { f_08868A34, 0x370, 0x28, 0x089096F8u, f_08909B68, 0x08909708u },
        { f_08867708, 0x358, 0x1, 0x08909720u, f_08909924, 0x08909730u },
        { f_08867708, 0x380, 0x6, 0x08909748u, f_08909EB0, 0x08909758u },
        { f_08867708, 0x388, 0xB, 0x08909770u, f_08909C50, 0x08909780u },
        { f_08867708, 0x378, 0xB, 0x08909798u, f_08909BDC, 0x089097A8u },
        { f_08867708, 0x390, 0x8, 0x089097C0u, f_08909CBC, 0x089097D0u },
        { f_0886A264, NONE, NONE, 0x089097E0u, f_08909A20, 0x089097F0u },
        { f_0886A2D8, NONE, NONE, 0x08909800u, f_08909D28, 0x08909810u },
    };
    run_checks(c, t, 12);
}
SFA_FUNC(0895C320, move_inputs_5c320) {
    static const Check t[12] = {
        { f_0886A1E4, NONE, NONE, 0x0895C334u, f_0895C594, 0x0895C344u },
        { f_08869008, 0x350, 0x35, 0x0895C35Cu, f_0895CB48, 0x0895C36Cu },
        { f_08867708, 0x358, 0x1B, 0x0895C384u, f_0895CAA8, 0x0895C394u },
        { f_08867708, 0x360, 0x18, 0x0895C3ACu, f_0895CA08, 0x0895C3BCu },
        { f_08867708, 0x368, 0x3, 0x0895C3D4u, f_0895C940, 0x0895C3E4u },
        { f_08867708, 0x370, 0x6, 0x0895C3FCu, f_0895C654, 0x0895C40Cu },
        { f_08867708, 0x378, 0x2, 0x0895C424u, f_0895C7A8, 0x0895C434u },
        { f_08867708, 0x380, 0xE, 0x0895C44Cu, f_0895C8BC, 0x0895C45Cu },
        { f_08867708, 0x388, 0x13, 0x0895C474u, f_0895C82C, 0x0895C484u },
        { f_08867708, 0x390, 0x4, 0x0895C49Cu, f_0895C6D8, 0x0895C4ACu },
        { f_0886A264, NONE, NONE, 0x0895C4BCu, f_0895CBD4, 0x0895C4CCu },
        { f_0886A2D8, NONE, NONE, 0x0895C4DCu, f_0895C4FC, 0x0895C4ECu },
    };
    run_checks(c, t, 12);
}
SFA_FUNC(0892A398, move_inputs_2a398) {
    static const Check t[13] = {
        { f_0886A1E4, NONE, NONE, 0x0892A3ACu, f_0892A7BC, 0x0892A3BCu },
        { f_08867708, 0x368, 0x1C, 0x0892A3D4u, f_0892A9D4, 0x0892A3E4u },
        { f_08867708, 0x370, 0x18, 0x0892A3FCu, f_0892AA80, 0x0892A40Cu },
        { f_08867708, 0x378, 0x1B, 0x0892A424u, f_0892A934, 0x0892A434u },
        { f_08867708, 0x358, 0x5, 0x0892A44Cu, f_0892AD04, 0x0892A45Cu },
        { f_08867708, 0x350, 0x1, 0x0892A474u, f_0892AC60, 0x0892A484u },
        { f_08867708, 0x360, 0x4, 0x0892A49Cu, f_0892AD84, 0x0892A4ACu },
        { f_0886A264, NONE, NONE, 0x0892A4BCu, f_0892AB8C, 0x0892A4CCu },
        { f_08868640, 0x398, 0x25, 0x0892A4E4u, f_0892A634, 0x0892A4F4u },
        { f_08868640, 0x390, 0x24, 0x0892A50Cu, f_0892A59C, 0x0892A51Cu },
        { f_08868640, 0x380, 0x22, 0x0892A534u, f_0892A6D8, 0x0892A544u },
        { f_08868640, 0x388, 0x23, 0x0892A55Cu, f_0892A748, 0x0892A56Cu },
        { f_0886A2D8, NONE, NONE, 0x0892A57Cu, f_0892A878, 0x0892A58Cu },
    };
    run_checks(c, t, 13);
}
SFA_FUNC(08989E3C, move_inputs_89e3c) {
    static const Check t[13] = {
        { f_0886A1E4, NONE, NONE, 0x08989E50u, f_0898A5EC, 0x08989E60u },
        { f_08867708, 0x380, 0x1C, 0x08989E78u, f_0898A164, 0x08989E88u },
        { f_08867708, 0x390, 0x18, 0x08989EA0u, f_0898A0D0, 0x08989EB0u },
        { f_08867708, 0x368, 0x5, 0x08989EC8u, f_0898A3CC, 0x08989ED8u },
        { f_08868A34, 0x398, 0x28, 0x08989EF0u, f_0898A568, 0x08989F00u },
        { f_08867708, 0x360, 0x10, 0x08989F18u, f_0898A454, 0x08989F28u },
        { f_08867708, 0x350, 0x1, 0x08989F40u, f_0898A4DC, 0x08989F50u },
        { f_08867708, 0x358, 0x6, 0x08989F68u, f_0898A344, 0x08989F78u },
        { f_08867708, 0x388, 0x7, 0x08989F90u, f_0898A040, 0x08989FA0u },
        { f_08867708, 0x370, 0x3, 0x08989FB8u, f_0898A1F8, 0x08989FC8u },
        { f_08867708, 0x378, 0x4, 0x08989FE0u, f_0898A284, 0x08989FF0u },
        { f_0886A264, NONE, NONE, 0x0898A000u, f_0898A69C, 0x0898A010u },
        { f_0886A2D8, NONE, NONE, 0x0898A020u, f_0898A7EC, 0x0898A030u },
    };
    run_checks(c, t, 13);
}
SFA_FUNC(088F71B0, move_inputs_f71b0) {
    static const Check t[15] = {
        { f_0886A1E4, NONE, NONE, 0x088F71C4u, f_088F7404, 0x088F71D4u },
        { f_08868488, 0x398, 0x21, 0x088F71ECu, f_088F7684, 0x088F71FCu },
        { f_08867708, 0x388, 0x1C, 0x088F7214u, f_088F77C4, 0x088F7224u },
        { f_08867708, 0x368, 0x1E, 0x088F723Cu, f_088F7B94, 0x088F724Cu },
        { f_08867708, 0x370, 0x17, 0x088F7264u, f_088F79C0, 0x088F7274u },
        { f_08867708, 0x378, 0x15, 0x088F728Cu, f_088F7950, 0x088F729Cu },
        { f_08867708, 0x380, 0x16, 0x088F72B4u, f_088F7878, 0x088F72C4u },
        { f_08867708, 0x358, 0x5, 0x088F72DCu, f_088F7DC4, 0x088F72ECu },
        { f_08867708, 0x3A8, 0xF, 0x088F7304u, f_088F74B8, 0x088F7314u },
        { f_08867708, 0x390, 0x13, 0x088F732Cu, f_088F7710, 0x088F733Cu },
        { f_08867708, 0x350, 0x1, 0x088F7354u, f_088F7CD0, 0x088F7364u },
        { f_08867708, 0x360, 0x4, 0x088F737Cu, f_088F7C30, 0x088F738Cu },
        { f_08867708, 0x3A0, 0x3, 0x088F73A4u, f_088F7530, 0x088F73B4u },
        { f_0886A264, NONE, NONE, 0x088F73C4u, f_088F7A54, 0x088F73D4u },
        { f_0886A2D8, NONE, NONE, 0x088F73E4u, f_088F75E0, 0x088F73F4u },
    };
    run_checks(c, t, 15);
}
