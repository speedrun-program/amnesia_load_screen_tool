
#pragma once


// macros generated with python 3:
/*
patterns_string = """
gpBase location            : e8 ?? ?? ?? ?? 89 45 ?? ?? ?? ?? ?? ?? 8b 88 ?? ?? ?? ?? e8 ?? ?? ?? ?? 8b 4e ?? 51 8b 8e     - plus 47
engineRun location         : d9 45 ?? 51 8b 4e ?? d9 1c 24 6a 05 e8 ?? ?? ?? ?? 8b 4e ?? e8 ?? ?? ?? ?? d9 5d ?? d9 45     - plus 40
m_lstSoundEntries offset   : 55 8b ec 56 8b f1 8b 46 ?? 8b 10 3b d0 74 ?? 53 8a 5d ?? 57 8b 7d                             - plus 8
cSoundEntryStop location   : 56 8b f1 80 7e ?? 00 75 ?? 80 7e ?? 00 74 ?? 8b 4e ?? 8b 01 8b 50 ?? ff d2 c6 46 ?? 00        - plus 0
isPlaying offset           : 80 7e ?? 00 74 ?? 8b 4e ?? 80 79 ?? 00 75 ?? 8b 4e ?? 8b 11 8b 42 ?? ff d0 c6 46 ?? 00        - plus 36
GetTotalTime offset        : d9 05 ?? ?? ?? ?? 8b 85 ?? ?? ?? ?? 8b 17 d9 9d ?? ?? ?? ?? d9 85 ?? ?? ?? ?? 50 8b 42        - plus 29
Sleep call                 : 75 ?? 6a 0a ff 15 ?? ?? ?? ?? 8b 46 ?? 8d 55 ?? 52 50 ff d7 81 7d ?? ?? ?? ?? ?? 75           - plus 4
strncmp call               : 6a 05 68 ?? ?? ?? ?? 53 ff 15 ?? ?? ?? ?? 83 c4 0c 85 c0 0f 84 ?? ?? ?? ?? 8b c6 66 c7 85     - plus 8
alt+f4 quit                : 8b 46 ?? 53 50 8b ce e8 ?? ?? ?? ?? d9 ee a1 ?? ?? ?? ?? 8b 08 8b 49 ?? 51 d9 1c 24 6a 09 e8  - plus 74
quit without saving        : 68 ?? ?? ?? ?? 8d 8d ?? ?? ?? ?? e8 ?? ?? ?? ?? 8b 16 8b 52 ?? ?? ?? ?? ?? ?? ?? 50 8b ce     - plus 21
quit and save              : 8b 8a ?? ?? ?? ?? e8 ?? ?? ?? ?? 68 ?? ?? ?? ?? 8d 8d ?? ?? ?? ?? e8 ?? ?? ?? ?? 8b 06 8b 50  - plus 32
DestroyMap call location   : 53 68 ?? ?? ?? ?? e8 ?? ?? ?? ?? 83 c4 08 85 f6 74 ?? 8b 0d ?? ?? ?? ?? 8b 89                 - plus 33
slippery physics           : 8b 17 89 9a ?? ?? ?? ?? 8b 07 ?? ?? ?? 89 88 ?? ?? ?? ?? 8b 17 8d 83 ?? ?? ?? ?? 50 8d 8b     - plus 10
"""


define_macro_strings = []


for line_idx, line in enumerate(patterns_string.strip().split("\n")):

    colon_idx = line.index(":")
    dash_idx = line.index("-", colon_idx)

    define_macro_strings.append([f"#define BYTE_SIGNATURE_{(line_idx + 1)}(m) ( \\\n    "])

    for byte_idx, byte_string in enumerate(line[colon_idx+1: dash_idx].split()):

        if byte_string == "??":
            continue

        if byte_idx == 0:
            define_macro_strings[line_idx].append(f"(m)[0] == 0x{byte_string} \\\n")
        else:
            define_macro_strings[line_idx].append(f"    && (m)[{byte_idx}] == 0x{byte_string} \\\n")

    define_macro_strings[line_idx].append(")\n\n")

    define_macro_strings[line_idx] = "".join(define_macro_strings[line_idx])


result_string = "".join(define_macro_strings)

print(result_string)
*/


// position 8 is manually added here with a unique check because that's where LiveSplit injects one of its jump
#define BYTE_SIGNATURE_1(m) ( \
    (m)[0] == 0xe8 \
    && (m)[5] == 0x89 \
    && (m)[6] == 0x45 \
    && ((m)[8] == 0xa1 || (m)[8] == 0xe9 || (m)[8] == 0xe8) \
    && (m)[13] == 0x8b \
    && (m)[14] == 0x88 \
    && (m)[19] == 0xe8 \
    && (m)[24] == 0x8b \
    && (m)[25] == 0x4e \
    && (m)[27] == 0x51 \
    && (m)[28] == 0x8b \
    && (m)[29] == 0x8e \
)

#define BYTE_SIGNATURE_2(m) ( \
    (m)[0] == 0xd9 \
    && (m)[1] == 0x45 \
    && (m)[3] == 0x51 \
    && (m)[4] == 0x8b \
    && (m)[5] == 0x4e \
    && (m)[7] == 0xd9 \
    && (m)[8] == 0x1c \
    && (m)[9] == 0x24 \
    && (m)[10] == 0x6a \
    && (m)[11] == 0x05 \
    && (m)[12] == 0xe8 \
    && (m)[17] == 0x8b \
    && (m)[18] == 0x4e \
    && (m)[20] == 0xe8 \
    && (m)[25] == 0xd9 \
    && (m)[26] == 0x5d \
    && (m)[28] == 0xd9 \
    && (m)[29] == 0x45 \
)

#define BYTE_SIGNATURE_3(m) ( \
    (m)[0] == 0x55 \
    && (m)[1] == 0x8b \
    && (m)[2] == 0xec \
    && (m)[3] == 0x56 \
    && (m)[4] == 0x8b \
    && (m)[5] == 0xf1 \
    && (m)[6] == 0x8b \
    && (m)[7] == 0x46 \
    && (m)[9] == 0x8b \
    && (m)[10] == 0x10 \
    && (m)[11] == 0x3b \
    && (m)[12] == 0xd0 \
    && (m)[13] == 0x74 \
    && (m)[15] == 0x53 \
    && (m)[16] == 0x8a \
    && (m)[17] == 0x5d \
    && (m)[19] == 0x57 \
    && (m)[20] == 0x8b \
    && (m)[21] == 0x7d \
)

#define BYTE_SIGNATURE_4(m) ( \
    (m)[0] == 0x56 \
    && (m)[1] == 0x8b \
    && (m)[2] == 0xf1 \
    && (m)[3] == 0x80 \
    && (m)[4] == 0x7e \
    && (m)[6] == 0x00 \
    && (m)[7] == 0x75 \
    && (m)[9] == 0x80 \
    && (m)[10] == 0x7e \
    && (m)[12] == 0x00 \
    && (m)[13] == 0x74 \
    && (m)[15] == 0x8b \
    && (m)[16] == 0x4e \
    && (m)[18] == 0x8b \
    && (m)[19] == 0x01 \
    && (m)[20] == 0x8b \
    && (m)[21] == 0x50 \
    && (m)[23] == 0xff \
    && (m)[24] == 0xd2 \
    && (m)[25] == 0xc6 \
    && (m)[26] == 0x46 \
    && (m)[28] == 0x00 \
)

#define BYTE_SIGNATURE_5(m) ( \
    (m)[0] == 0x80 \
    && (m)[1] == 0x7e \
    && (m)[3] == 0x00 \
    && (m)[4] == 0x74 \
    && (m)[6] == 0x8b \
    && (m)[7] == 0x4e \
    && (m)[9] == 0x80 \
    && (m)[10] == 0x79 \
    && (m)[12] == 0x00 \
    && (m)[13] == 0x75 \
    && (m)[15] == 0x8b \
    && (m)[16] == 0x4e \
    && (m)[18] == 0x8b \
    && (m)[19] == 0x11 \
    && (m)[20] == 0x8b \
    && (m)[21] == 0x42 \
    && (m)[23] == 0xff \
    && (m)[24] == 0xd0 \
    && (m)[25] == 0xc6 \
    && (m)[26] == 0x46 \
    && (m)[28] == 0x00 \
)

#define BYTE_SIGNATURE_6(m) ( \
    (m)[0] == 0xd9 \
    && (m)[1] == 0x05 \
    && (m)[6] == 0x8b \
    && (m)[7] == 0x85 \
    && (m)[12] == 0x8b \
    && (m)[13] == 0x17 \
    && (m)[14] == 0xd9 \
    && (m)[15] == 0x9d \
    && (m)[20] == 0xd9 \
    && (m)[21] == 0x85 \
    && (m)[26] == 0x50 \
    && (m)[27] == 0x8b \
    && (m)[28] == 0x42 \
)

#define BYTE_SIGNATURE_7(m) ( \
    (m)[0] == 0x75 \
    && (m)[2] == 0x6a \
    && (m)[3] == 0x0a \
    && (m)[4] == 0xff \
    && (m)[5] == 0x15 \
    && (m)[10] == 0x8b \
    && (m)[11] == 0x46 \
    && (m)[13] == 0x8d \
    && (m)[14] == 0x55 \
    && (m)[16] == 0x52 \
    && (m)[17] == 0x50 \
    && (m)[18] == 0xff \
    && (m)[19] == 0xd7 \
    && (m)[20] == 0x81 \
    && (m)[21] == 0x7d \
    && (m)[27] == 0x75 \
)

#define BYTE_SIGNATURE_8(m) ( \
    (m)[0] == 0x6a \
    && (m)[1] == 0x05 \
    && (m)[2] == 0x68 \
    && (m)[7] == 0x53 \
    && (m)[8] == 0xff \
    && (m)[9] == 0x15 \
    && (m)[14] == 0x83 \
    && (m)[15] == 0xc4 \
    && (m)[16] == 0x0c \
    && (m)[17] == 0x85 \
    && (m)[18] == 0xc0 \
    && (m)[19] == 0x0f \
    && (m)[20] == 0x84 \
    && (m)[25] == 0x8b \
    && (m)[26] == 0xc6 \
    && (m)[27] == 0x66 \
    && (m)[28] == 0xc7 \
    && (m)[29] == 0x85 \
)

#define BYTE_SIGNATURE_9(m) ( \
    (m)[0] == 0x8b \
    && (m)[1] == 0x46 \
    && (m)[3] == 0x53 \
    && (m)[4] == 0x50 \
    && (m)[5] == 0x8b \
    && (m)[6] == 0xce \
    && (m)[7] == 0xe8 \
    && (m)[12] == 0xd9 \
    && (m)[13] == 0xee \
    && (m)[14] == 0xa1 \
    && (m)[19] == 0x8b \
    && (m)[20] == 0x08 \
    && (m)[21] == 0x8b \
    && (m)[22] == 0x49 \
    && (m)[24] == 0x51 \
    && (m)[25] == 0xd9 \
    && (m)[26] == 0x1c \
    && (m)[27] == 0x24 \
    && (m)[28] == 0x6a \
    && (m)[29] == 0x09 \
    && (m)[30] == 0xe8 \
)

#define BYTE_SIGNATURE_10(m) ( \
    (m)[0] == 0x68 \
    && (m)[5] == 0x8d \
    && (m)[6] == 0x8d \
    && (m)[11] == 0xe8 \
    && (m)[16] == 0x8b \
    && (m)[17] == 0x16 \
    && (m)[18] == 0x8b \
    && (m)[19] == 0x52 \
    && (m)[27] == 0x50 \
    && (m)[28] == 0x8b \
    && (m)[29] == 0xce \
)

#define BYTE_SIGNATURE_11(m) ( \
    (m)[0] == 0x8b \
    && (m)[1] == 0x8a \
    && (m)[6] == 0xe8 \
    && (m)[11] == 0x68 \
    && (m)[16] == 0x8d \
    && (m)[17] == 0x8d \
    && (m)[22] == 0xe8 \
    && (m)[27] == 0x8b \
    && (m)[28] == 0x06 \
    && (m)[29] == 0x8b \
    && (m)[30] == 0x50 \
)

#define BYTE_SIGNATURE_12(m) ( \
    (m)[0] == 0x53 \
    && (m)[1] == 0x68 \
    && (m)[6] == 0xe8 \
    && (m)[11] == 0x83 \
    && (m)[12] == 0xc4 \
    && (m)[13] == 0x08 \
    && (m)[14] == 0x85 \
    && (m)[15] == 0xf6 \
    && (m)[16] == 0x74 \
    && (m)[18] == 0x8b \
    && (m)[19] == 0x0d \
    && (m)[24] == 0x8b \
    && (m)[25] == 0x89 \
)

#define BYTE_SIGNATURE_13(m) ( \
    (m)[0] == 0x8b \
    && (m)[1] == 0x17 \
    && (m)[2] == 0x89 \
    && (m)[3] == 0x9a \
    && (m)[8] == 0x8b \
    && (m)[9] == 0x07 \
    && (m)[13] == 0x89 \
    && (m)[14] == 0x88 \
    && (m)[19] == 0x8b \
    && (m)[20] == 0x17 \
    && (m)[21] == 0x8d \
    && (m)[22] == 0x83 \
    && (m)[27] == 0x50 \
    && (m)[28] == 0x8d \
    && (m)[29] == 0x8b \
)
