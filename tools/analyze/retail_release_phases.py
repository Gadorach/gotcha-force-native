"""Reviewed phase-byte transitions to projectile call sites, not phase execution."""

# Each row pins the dispatch handler through the target leaf's return, the
# exact table prefix, one actor+540 advance per predecessor, and the reviewed
# calls in the final leaf. None of the advances is assumed to be unconditional.
# Call value: positive model requires equality; negative model excludes it;
# zero means no model comparison at this call site.
WINDOWS = {
    # This small-data table has only two reviewed entries. The following
    # halfwords are not another release phase.
    0x804346F8: (
        0x8015A8C0,(0x8015A8F8,0x8015AA24),0x8015AC38,(0x8015A918,),
        "dde0dd5658c1781c593f27be89f27d8f7a5650018a1c038278aa848aaebba776",
        {0x8015AACC:0x0610,0x8015AAE4:0x061E,0x8015AAFC:0x0621,0x8015AB14:0x0623}),
    0x802D65D0: (
        0x80079AB8,(0x80079B08,0x80079BA8,0x80079C3C),0x80079D00,(0x80079B24,0x80079C24),
        "6252e6127e4438f7d64fa32a5a0f0022b83f622d3b8ae7302f2e635bbb323010",
        {0x80079CC8:0x0900,0x80079CD8:0x0908,0x80079CE8:0x090D}),
    0x8031D158: (
        0x800F7F98,(0x800F7FF4,0x800F800C,0x800F809C),0x800F80EC,(0x800F8000,0x800F8084),
        "93a2a176b1971e2fde834128f29150c99428d2848a10dfabfee3ef55bb9f2fe5",
        {0x800F80CC:0x0D00,0x800F80D8:-0x0D00}),
    0x803216A4: (
        0x80103C10,(0x80103C60,0x80103D88),0x80103E7C,(0x80103C7C,),
        "df6e2491fc920ba7c73f31080ee17250e597c3f1b69b525ca4d9aa1622895577",
        {0x80103E14:0,0x80103E20:0}),
    0x80321FE8: (
        0x80105F68,(0x80105FA4,0x80105FBC,0x80106028),0x8010609C,(0x80105FB0,0x80106000),
        "0a3bff2f04edda7592aa1d8eaf81012161b4b640519d90955079341850939302",
        {0x80106064:0,0x80106070:0}),
    0x80321FF8: (
        0x801060EC,(0x80106128,0x80106140,0x801061AC),0x80106220,(0x80106134,0x80106184),
        "8574d8185a44f67d460decd3bf0dc7b5eb9ab23b36d6ad3f8f03bf6e654e1eee",
        {0x801061E8:0,0x801061F4:0}),
    0x80337FB8: (
        0x80135B98,(0x80135BE8,0x80135CE8),0x80135DE4,(0x80135C04,),
        "879b987c03143fa7967a8a4a3b01230df53915ad47109b8b0e70460b99ab7fba",
        {0x80135DB0:0}),
    0x80340E48: (
        0x8014D878,(0x8014D8EC,0x8014D934,0x8014D9C0),0x8014DA3C,(0x8014D910,0x8014D998),
        "55687de2e957283bca2b1d471f1dccc5f973c2f32c507f05d2e7af1b05257f17",
        {0x8014DA24:0}),
    0x80340E74: (
        0x8014DFB0,(0x8014E000,0x8014E0A8,0x8014E178),0x8014E258,(0x8014E024,0x8014E108),
        "e783f5a0fe15ca8adb0aabb2c1dedf393953f8e3fb75ef8b161ce19710a19ad4",
        {0x8014E1DC:0}),
    0x80341E88: (
        0x80151D58,(0x80151DB0,0x80151E50,0x80151ED4),0x80151F7C,(0x80151DCC,0x80151EBC),
        "dfe7193178f1e411cad25b96429075c79c137776c79e00c2b7488f60313dd602",
        {0x80151F54:0x0903,0x80151F64:0x090A}),
    0x80348B98: (
        0x8015DC88,(0x8015DCFC,0x8015DD44,0x8015DDD0),0x8015DE78,(0x8015DD20,0x8015DDA8),
        "3742b666017e2b764e7e9045c4cb02204c20fd57907f6ca8c1b2a2720bfe808b",
        {0x8015DE50:0x0902,0x8015DE60:0x0909}),
    0x80348BC4: (
        0x8015E444,(0x8015E494,0x8015E53C,0x8015E60C),0x8015E718,(0x8015E4B8,0x8015E59C),
        "d64b719538de6984c3908c6b7be20f7e13d4e99d4f727d322fef2e56274aa22c",
        {0x8015E68C:0x0902,0x8015E69C:0x0909}),
    0x8036299C: (
        0x801892B8,(0x801892F4,0x80189320,0x801893B4),0x801894C0,(0x80189304,0x80189394),
        "b7e565ab052a119ef8b362c57391794b9d1614e67c2472b40f4bd38f0b4400e7",
        {0x80189420:0,0x8018942C:0}),
    0x80380A80: (
        0x801B29F0,(0x801B2A2C,0x801B2A54,0x801B2AC8),0x801B2B28,(0x801B2A3C,0x801B2AB0),
        "85cab6e5feb043062f4612f0d45e99dc9498c46e81f47b7066b16846607fa16a",
        {0x801B2AFC:0}),
    0x80380EE8: (
        0x801B372C,(0x801B3768,0x801B3798,0x801B3824),0x801B3990,(0x801B377C,0x801B380C),
        "79971d37b6d1691b2f902a1b618cb71af154f8605bc8f1eafda9d3e58209fe09",
        {0x801B38B0:0x0F01,0x801B394C:0x0F02}),
    0x803816D0: (
        0x801B4280,(0x801B42F4,0x801B433C,0x801B43C8),0x801B4470,(0x801B4318,0x801B43A0),
        "1d4eb9e589c342a7f7764955fc351f4a48cc25f501b182301b3e73cc8dca92ef",
        {0x801B4448:0x0907,0x801B4458:0x090C}),
    0x803816FC: (
        0x801B4A3C,(0x801B4A8C,0x801B4B34,0x801B4C04),0x801B4D10,(0x801B4AB0,0x801B4B94),
        "ded652fd1379d83cd85b9713912f2884ee0b2e5b05792737adb5b6a91cd07d38",
        {0x801B4C84:0x0907,0x801B4C94:0x090C}),
    0x80391CD8: (
        0x801E45D0,(0x801E460C,0x801E463C,0x801E46C8),0x801E47F8,(0x801E4620,0x801E46B0),
        "cf8a99b4ea463f7a14379a63917390df5d184a1f4804ac57a7bd577d143cbfd2",
        {0x801E4754:0x0F06,0x801E4764:0x0F06}),
}

# A single press test can use this first paid pair, not the later repeats.
# model -> (phase table, final helper, ordered factory keys and sites)
STATIC_GROUND_PHASE_PAIRS = {
    0x0D03:(0x8036299C,0x801893B4,(54,55),(0x80189420,0x8018942C)),
}

# Violet Attacker: a zero seed, then alternation after accepted debits.
# The bounded window includes the wrapper and its first three phase leaves.
VIOLET_SHOT = {
    "model":0x0D05,"table":0x80362664,"wrapper":0x80188B60,"leaves":(0x80188B9C,0x80188BC8,0x80188C54),
    "end":0x80188D5C,"digest":"90e07f3b5c3ab8899481d7cae33f59729a933428f5d8ca5a3d5268f46271ec1d",
    "stores":(0x80188BAC,0x80188C3C),"site":0x80188CC8,"debit":0x80188CAC,"keys":(16,17),
}

# Two-entry small-data tables with the phase-zero write in a shared helper.
# (wrapper, entry, consumer, wrapper extent end, digest,
#  entry-to-helper call site, helper, factory call site, model)
HELPER_WINDOWS = {
    0x804338A0:(0x80076AA0,0x80076AD8,0x80076B48,0x80076C80,
                "5377ac407f570030f3b6c36c0431f2711ac67b9e0e9e95296aa164b7093d5223",
                0x80076B30,0x80076C80,0x80076C10,0x0800),
    0x804338A8:(0x80076CD4,0x80076D0C,0x80076B48,0x80076D64,
                "6efbb53ed1ce4db983fb588ac435e8fef0a4eda367364777dba74c95a4e737d1",
                0x80076D4C,0x80076C80,0x80076C10,0x0800),
    0x804338B0:(0x80076D64,0x80076D9C,0x80076E00,0x80076F78,
                "42f47feccbbfdca679a2b11022856d17f8e8b061a964133ea8ea8f99924d6e45",
                0x80076DD0,0x80076C80,0x80076EFC,0x0800),
    0x804338B8:(0x80076F78,0x80076FB0,0x80077018,0x80077144,
                "6f5f6847e389a0d1660ed1cfa68004e4b4e09ddffd1c89b2a34e9ad30af1dea9",
                0x80076FF0,0x80076C80,0x800770D0,0x0800),
    0x80433A38:(0x800A3ECC,0x800A3F04,0x800A3F74,0x800A40AC,
                "1af0d4a87b418e35a34a967e85ebf60610809b0bfa0bce53368db56ff51520a4",
                0x800A3F5C,0x800A40AC,0x800A403C,0x0801),
    0x80433A40:(0x800A4100,0x800A4138,0x800A3F74,0x800A4190,
                "b0a05fbae66c32cbe35536ad730f39fab550aafd2d2c129e730e5ebba992bf55",
                0x800A4178,0x800A40AC,0x800A403C,0x0801),
    0x80433A48:(0x800A4190,0x800A41C8,0x800A422C,0x800A43A4,
                "0fddc56777f5ec0216e0969a7925b2a3e26127b791bb3b5ea75dac646d99f848",
                0x800A41FC,0x800A40AC,0x800A4328,0x0801),
    0x80433A50:(0x800A43A4,0x800A43DC,0x800A4444,0x800A4570,
                "5a25cda02175e61feb26e1369ca7a4e0397c323226656e02615e7aed884ee8e1",
                0x800A441C,0x800A40AC,0x800A44FC,0x0801),
}
ADVANCE_HELPERS = {
    0x80076C80:(0x54,"5f6e5ad7390559a70e56f70551636b1b521286b4e63373e7a6f91903f331e145",0x80076CA0),
    0x800A40AC:(0x54,"3c6a9805011a8d0dcecbdce4df22a43b0b0e40445acd694e730611860424967b",0x800A40CC),
}
# Some wrappers share an earlier consumer outside the pinned wrapper span.
SEPARATE_CONSUMERS = {
    0x80076B48:(0x138,"1c5c886c96b72153665e0f583988bdb0c96771aac688608f026ab56bfda630c3"),
    0x800A3F74:(0x138,"a2f1b02c5b263a359d43208bbdfcae0493970bf445baeffa4d03b0ec66b848a8"),
}

RELEASE_GUARDS = {
    0x8015AA24: {"actor_signed_byte_positive":"1CEF","clear_before_request":"8015AA9C",
                 "requires_nonzero_return_from":"8006DBE0"},
}
