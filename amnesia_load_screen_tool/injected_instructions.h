
#pragma once


unsigned char mainMenuDelayInstructions[64] = {

    0x68, 0x00, 0x00, 0x00, 0x00,                // push mainMenuDelay
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          // call Sleep // this undoes the last push
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          // altf4QuitBytes
    0xc3,                                        // ret

    0x52,                                        // push edx
    0x68, 0x00, 0x00, 0x00, 0x00,                // push mainMenuDelay
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          // call Sleep // this undoes the last push
    0x5a,                                        // pop edx
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          // noSaveQuitBytes
    0xc3,                                        // ret

    0x52,                                        // push edx
    0x68, 0x00, 0x00, 0x00, 0x00,                // push mainMenuDelay
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          // call Sleep // this undoes the last push
    0x5a,                                        // pop edx
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          // saveQuitBytes
    0xc3,                                        // ret

    0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,          // int3 filler so this is 64 bytes
};


unsigned char mapDelayInstructions[128] = {

    // entry point if quickloading
    0xe8, 0x00, 0x00, 0x00, 0x00,                // call DestroyMap
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    // copied bytes from loadFromMenuBytes
    0xe9, 0x00, 0x00, 0x00, 0x00,                // jmp back to amnesia (after copied bytes)

    0xcc,                                        // int3 filler so the loop is aligned by 64 bytes

    // entry point if not quickloading
    0x53,                                        // push ebx
    0x53,                                        // push esi
    0x57,                                        // push edi
    0xbb, 0x00, 0x00, 0x00, 0x00,                // mov ebx, spacePerMapName
    0x8b, 0x35, 0x00, 0x00, 0x00, 0x00,          // mov esi, dword ptr [strncmp pointer]
    0x89, 0xf8,                                  // mov eax, edi // moving the map name std::string into eax

    // if the std::string is size 15 or less, the c-string is stored in the first 16 bytes of the std::string.
    // otherwise, the c-string is dynamically allocated and accessed through a pointer stored at the beginning of the std::string.
    0x83, 0x7f, 0x14, 0x10,                      // cmp dword ptr [edi + 20], 16
    0x72, 0x02,                                  // jb 2
    0x8b, 0x07,                                  // mov eax, dword ptr [edi]

    0xbf, 0x00, 0x00, 0x00, 0x00,                // mov edi, noMoreMapNamesAddress
    0x50,                                        // push eax (map name c-string)
    0x68, 0x00, 0x00, 0x00, 0x00,                // push firstMapNameAddress
    0x53,                                        // push ebx (spacePerMapName)
    0x50,                                        // push eax (map name c-string)
    0xff, 0x74, 0x24, 0x08,                      // push dword ptr [esp + 8] (firstMapNameAddress)
    0x39, 0x3c, 0x24,                            // cmp dword ptr [esp], edi (noMoreMapNamesAddress)
    0x73, 0x1a,                                  // jnb to loop end

    // loop start
    0xff, 0xd6,                                  // call esi (strncmp)
    0x85, 0xc0,                                  // test eax, eax
    0x74, 0x16,                                  // jz to Sleep call
    0x83, 0xc4, 0x0c,                            // add esp, 12
    0x01, 0x1c, 0x24,                            // add dword ptr [esp], ebx
    0x53,                                        // push ebx (spacePerMapName)
    0xff, 0x74, 0x24, 0x08,                      // push dword ptr [esp + 8] (map name c-string)
    0xff, 0x74, 0x24, 0x08,                      // push dword ptr [esp + 8] (next map name to compare against)
    0x39, 0x3c, 0x24,                            // cmp dword ptr [esp], edi (noMoreMapNamesAddress) (precautionary check)
    0x72, 0xe6,                                  // jb to loop start
    // loop end

    0xeb, 0x0f,                                  // jmp to after Sleep call
    // Sleep call
    0x8b, 0x04, 0x24,                            // mov eax, dword ptr [esp]
    0xff, 0xb0, 0x00, 0x00, 0x00, 0x00,          // push dword ptr [eax + delay offset]
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          // call Sleep // this undoes the last push
    // after Sleep call

    0x83, 0xc4, 0x14,                            // add esp, 20
    0x5f,                                        // pop edi
    0x5e,                                        // pop esi
    0x5b,                                        // pop ebx
    0x00, 0x00, 0x00, 0x00, 0x00,                // the last 5 copied bytes from loadFromMenuBytes
    0xe9, 0x00, 0x00, 0x00, 0x00,                // jmp back to amnesia (after copied bytes)

    0xcc, 0xcc, 0xcc, 0xcc, 0xcc,                // int3 filler so this is 128 bytes
};


unsigned char flashbackSkipInstructions[208] = {

    // int3 filler so the loop is aligned by 64 bytes
    0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
    0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
    0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
    0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,

    0xa1, 0x00, 0x00, 0x00, 0x00,                // mov eax, dword ptr [gpBaseLocation]
    0x8b, 0x00,                                  // mov eax, dword ptr [eax] (mpEngine)
    0x8b, 0x40, 0x00,                            // mov eax, dword ptr [eax + gpBaseMpSoundOffset]
    0x8b, 0x40, 0x00,                            // mov eax, dword ptr [eax + mpSoundHandlerOffset]
    0x50,                                        // push eax (cSoundHandler pointer)
    0x53,                                        // push ebx
    0x56,                                        // push esi
    0x57,                                        // push edi
    0x8b, 0x70, 0x00,                            // mov esi, dword ptr [eax + m_lstSoundEntries offset]
    0x8b, 0x1d, 0x00, 0x00, 0x00, 0x00,          // mov ebx, dword ptr [strncmp pointer]
    0x8b, 0x3e,                                  // mov edi, dword ptr [esi] (first node, or the start of the list if it's empty)

    // the start of the m_lstSoundEntries list is used to indicate the end of the list
    0x39, 0xf7,                                  // cmp edi, esi
    0x74, 0x7a,                                  // jz to outer loop end

    // outer loop start
    0x8b, 0x4f, 0x00,                            // mov ecx, dword ptr [edi + nodeCSoundEntryOffset]
    0x85, 0xc9,                                  // test ecx, ecx (null pointer check)
    0x74, 0x6d,                                  // jz to inner loop end and past "add esp, 16"
    0x89, 0xc8,                                  // mov eax, ecx

    // the std::string object should be at the front of the cSoundEntry object

    // if the std::string is size 15 or less, the c-string is stored in the first 16 bytes of the std::string.
    // otherwise, the c-string is dynamically allocated and accessed through a pointer stored at the beginning of the std::string.
    0x83, 0x79, 0x14, 0x10,                      // cmp dword ptr [ecx + 20], 16
    0x72, 0x02,                                  // jb 2
    0x8b, 0x01,                                  // mov eax, dword ptr [ecx]

    // saving the cSoundEntry pointer and some arguments for the inner loop strncmp call
    0x51,                                        // push ecx (cSoundEntry object)
    0x68, 0x00, 0x00, 0x00, 0x00,                // push spacePerFlashbackName
    0x50,                                        // push eax (sound name c-string)

    // checking the prefix
    0x68, 0x00, 0x00, 0x00, 0x00,                // push lengthOfCommonPrefix
    0x50,                                        // push eax (sound name c-string)
    0x68, 0x00, 0x00, 0x00, 0x00,                // push commonPrefixAddress
    0xff, 0xd3,                                  // call ebx (strncmp)
    0x83, 0xc4, 0x0c,                            // add esp, 12
    0x81, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00,    // add dword ptr [esp], lengthOfCommonPrefix
    0x68, 0x00, 0x00, 0x00, 0x00,                // push firstFlashbackNameAddress
    0x85, 0xc0,                                  // test eax, eax
    0x75, 0x39,                                  // jnz to inner loop end
    0x81, 0x3c, 0x24, 0x00, 0x00, 0x00, 0x00,    // cmp dword ptr [esp], noMoreFlashbackNamesAddress (precautionary check)
    0x73, 0x30,                                  // jnb to inner loop end

    // inner loop start
    0xff, 0x74, 0x24, 0x08,                      // push dword ptr [esp + 8]
    0xff, 0x74, 0x24, 0x08,                      // push dword ptr [esp + 8]
    0xff, 0x74, 0x24, 0x08,                      // push dword ptr [esp + 8]
    0xff, 0xd3,                                  // call ebx (strncmp)
    0x83, 0xc4, 0x0c,                            // add esp, 12
    0x85, 0xc0,                                  // test eax, eax
    0x75, 0x0b,                                  // jnz to after the cSoundEntry::Stop call

    // calling cSoundEntry::Stop
    0x8b, 0x4c, 0x24, 0x0c,                      // mov ecx, dword ptr [esp + 12] (cSoundEntry object)
    0xe8, 0x00, 0x00, 0x00, 0x00,                // call cSoundEntry::Stop
    0xeb, 0x10,                                  // jmp to inner loop end

    // preparing for the next inner loop
    0x81, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00,    // add dword ptr [esp], spacePerFlashbackName
    0x81, 0x3c, 0x24, 0x00, 0x00, 0x00, 0x00,    // cmp dword ptr [esp], noMoreFlashbackNamesAddress
    0x72, 0xd0,                                  // jb to inner loop start
    // inner loop end

    0x83, 0xc4, 0x10,                            // add esp, 16
    0x8b, 0x3f,                                  // mov edi, dword ptr [edi]
    0x39, 0xf7,                                  // cmp edi, esi
    0x75, 0x86,                                  // jnz to outer loop start
    // outer loop end

    0x5f,                                        // pop edi
    0x5e,                                        // pop esi
    0x5b,                                        // pop ebx
    0x58,                                        // pop eax
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          // copied bytes from beforeFadeOutAllBytes
    0xe9, 0x00, 0x00, 0x00, 0x00,                // jmp back to amnesia (after where cSoundHandler is in eax)

    // int3 filler to multiple of 16
    0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
};


unsigned char flashbackWaitInstructions[336] = {

    // entry point from cLuxMapHandler::CheckMapChange
    0xb1, 0x01,                                  // mov cl, 1
    0x86, 0x0d, 0x00, 0x00, 0x00, 0x00,          // xchg cl, byte ptr [waitForFlashbackByteLocation]
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          // copied bytes from beforeFadeOutAllBytes
    0xc3,                                        // ret

    // int3 filler
    // make sure entry point from cEngine::Run will be aligned by 16
    0xcc,

    // entry point from cEngine::Run
    0xe8, 0x00, 0x00, 0x00, 0x00,                // call cEngine::GetStepSize
    0xa0, 0x00, 0x00, 0x00, 0x00,                // mov al, byte ptr [waitForFlashbackByteLocation]
    0x84, 0xc0,                                  // test al, al
    0x75, 0x01,                                  // jnz over the ret
    0xc3,                                        // ret

    0x56,                                        // push esi
    0x57,                                        // push edi
    0x57,                                        // push edi // dummy push to save the float returned by cEngine::GetStepSize
    0xd9, 0x1c, 0x24,                            // fstp dword ptr [esp]
    0x31, 0xc0,                                  // xor eax, eax
    0x86, 0x05, 0x00, 0x00, 0x00, 0x00,          // xchg al, byte ptr [waitForFlashbackByteLocation]
    0xa1, 0x00, 0x00, 0x00, 0x00,                // mov eax, dword ptr [gpBaseLocation]
    0x8b, 0x30,                                  // mov esi, dword ptr [eax] (mpEngine)
    0x8b, 0x76, 0x00,                            // mov esi, dword ptr [esi + gpBaseMpSoundOffset]
    0x8b, 0x76, 0x00,                            // mov esi, dword ptr [esi + mpSoundHandlerOffset]
    0x8b, 0x76, 0x00,                            // mov esi, dword ptr [esi + m_lstSoundEntries offset]
    0x8b, 0x3e,                                  // mov edi, dword ptr [esi] (first node, or the start of the list if it's empty)

    // the start of the m_lstSoundEntries list is used to indicate the end of the list
    0x39, 0xf7,                                  // cmp edi, esi
    0x0f, 0x84, 0xf5, 0x00, 0x00, 0x00,          // jz to "fld dword ptr [esp]" near the jump back to amnesia

    0x53,                                        // push ebx
    0x8b, 0x1d, 0x00, 0x00, 0x00, 0x00,          // mov ebx, dword ptr [strncmp pointer]

    // making space for two doubles initialized to zero
    0x31, 0xc0,                                  // xor eax, eax
    0x50,                                        // push eax
    0x50,                                        // push eax
    0x50,                                        // push eax
    0x50,                                        // push eax

    // outer loop start
    0x8b, 0x4f, 0x00,                            // mov ecx, dword ptr [edi + nodeCSoundEntryOffset]
    0x85, 0xc9,                                  // test ecx, ecx (null pointer check)
    0x0f, 0x84, 0xa8, 0x00, 0x00, 0x00,          // jz to inner loop end and past the first "add esp, 16"
    0x89, 0xc8,                                  // mov eax, ecx

    // the std::string object should be at the front of the cSoundEntry object

    // if the std::string is size 15 or less, the c-string is stored in the first 16 bytes of the std::string.
    // otherwise, the c-string is dynamically allocated and accessed through a pointer stored at the beginning of the std::string.
    0x83, 0x79, 0x14, 0x10,                      // cmp dword ptr [ecx + 20], 16
    0x72, 0x02,                                  // jb 2
    0x8b, 0x01,                                  // mov eax, dword ptr [ecx]

    // saving the cSoundEntry pointer and some arguments for the inner loop strncmp call
    0x51,                                        // push ecx (cSoundEntry object)
    0x68, 0x00, 0x00, 0x00, 0x00,                // push spacePerFlashbackName
    0x50,                                        // push eax (sound name c-string)

    // checking the prefix
    0x68, 0x00, 0x00, 0x00, 0x00,                // push lengthOfCommonPrefix
    0x50,                                        // push eax (sound name c-string)
    0x68, 0x00, 0x00, 0x00, 0x00,                // push commonPrefixAddress
    0xff, 0xd3,                                  // call ebx (strncmp)
    0x83, 0xc4, 0x0c,                            // add esp, 12
    0x81, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00,    // add dword ptr [esp], lengthOfCommonPrefix
    0x68, 0x00, 0x00, 0x00, 0x00,                // push firstFlashbackNameAddress
    0x85, 0xc0,                                  // test eax, eax
    0x75, 0x74,                                  // jnz to inner loop end
    0x81, 0x3c, 0x24, 0x00, 0x00, 0x00, 0x00,    // cmp dword ptr [esp], noMoreFlashbackNamesAddress (precautionary check)
    0x73, 0x6b,                                  // jnb to inner loop end

    // inner loop start
    0xff, 0x74, 0x24, 0x08,                      // push dword ptr [esp + 8]
    0xff, 0x74, 0x24, 0x08,                      // push dword ptr [esp + 8]
    0xff, 0x74, 0x24, 0x08,                      // push dword ptr [esp + 8]
    0xff, 0xd3,                                  // call ebx (strncmp)
    0x83, 0xc4, 0x0c,                            // add esp, 12
    0x85, 0xc0,                                  // test eax, eax
    0x75, 0x46,                                  // jnz to after storing the remaining time

    // storing how much time is left for this flashback line to finish
    0x56,                                        // push esi
    0x57,                                        // push edi
    0x8b, 0x4c, 0x24, 0x14,                      // mov ecx, dword ptr [esp + 20] (cSoundEntry object)
    0x8b, 0x49, 0x00,                            // mov ecx, dword ptr [ecx + soundChannelOffset] (cSoundEntry's iSoundChannel object)
    0x85, 0xc9,                                  // test ecx, ecx (null pointer check)
    0x74, 0x35,                                  // jz to pop edi
    0x89, 0xce,                                  // mov esi, ecx
    0x8b, 0x3e,                                  // mov edi, dword ptr [esi] (iSoundChannel vtable)
    0x8a, 0x46, 0x00,                            // mov al, byte ptr [esi + getPausedOffset]
    0x22, 0x46, 0x00,                            // and al, byte ptr [esi + getLoopingOffset]
    0x75, 0x29,                                  // jnz to pop edi
    0xff, 0x57, 0x00,                            // call dword ptr [edi + isPlayingOffset]
    0x84, 0xc0,                                  // test al, al
    0x74, 0x22,                                  // jz to pop edi
    0x89, 0xf1,                                  // mov ecx, esi
    0xff, 0x57, 0x00,                            // call dword ptr [edi + getElapsedTimeOffset]
    0x89, 0xf1,                                  // mov ecx, esi
    0xdd, 0x5c, 0x24, 0x20,                      // fstp qword ptr [esp + 32]
    0xff, 0x57, 0x00,                            // call dword ptr [edi + getTotalTimeOffset]
    0xdc, 0x64, 0x24, 0x20,                      // fsub qword ptr [esp + 32]
    0xdd, 0x44, 0x24, 0x18,                      // fld qword ptr [esp + 24]
    0xdf, 0xf1,                                  // fcomip st(0), st(1)
    0x73, 0x06,                                  // jnb to fstp st(0) (already waiting for a different sound with more remaining time)
    0xdd, 0x5c, 0x24, 0x18,                      // fstp qword ptr [esp + 24]
    0xeb, 0x02,                                  // jmp to pop edi
    0xdd, 0xd8,                                  // fstp st(0)
    0x5f,                                        // pop edi
    0x5e,                                        // pop esi
    0xeb, 0x10,                                  // jmp to inner loop end

    // preparing for the next inner loop
    0x81, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00,    // add dword ptr [esp], spacePerFlashbackName
    0x81, 0x3c, 0x24, 0x00, 0x00, 0x00, 0x00,    // cmp dword ptr [esp], noMoreFlashbackNamesAddress
    0x72, 0x95,                                  // jb to inner loop start
    // inner loop end

    0x83, 0xc4, 0x10,                            // add esp, 16
    0x8b, 0x3f,                                  // mov edi, dword ptr [edi]
    0x39, 0xf7,                                  // cmp edi, esi
    0x0f, 0x85, 0x43, 0xff, 0xff, 0xff,          // jnz to outer loop start

    // checking if any flashback lines are close enough to finishing to let the load screen end
    0xdd, 0x04, 0x24,                            // fld qword ptr [esp] (time remaining before flashback end)
    0xdd, 0x05, 0x00, 0x00, 0x00, 0x00,          // fld qword ptr [secondsRemainingBeforeUnwaitAddress] // this should be at least 0.001
    0xdf, 0xf1,                                  // fcomip st(0), st(1)
    0xdd, 0xd8,                                  // fstp st(0)
    0x73, 0x18,                                  // jnb to outer loop end
    0x6a, 0x01,                                  // push 1
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          // call Sleep // this undoes the last push
    0x8b, 0x3e,                                  // mov edi, dword ptr [esi] (first node, or the start of the list if it's empty)

    // resetting the memory to store doubles to zero
    0x83, 0xc4, 0x10,                            // add esp, 16
    0x31, 0xc0,                                  // xor eax, eax
    0x50,                                        // push eax
    0x50,                                        // push eax
    0x50,                                        // push eax
    0x50,                                        // push eax
    0xe9, 0x1c, 0xff, 0xff, 0xff,                // jmp to outer loop start
    // outer loop end

    0x83, 0xc4, 0x10,                            // add esp, 16
    0x5b,                                        // pop ebx
    0xd9, 0x04, 0x24,                            // fld dword ptr [esp]
    0x5f,                                        // pop edi // dummy pop
    0x5f,                                        // pop edi
    0x5e,                                        // pop esi
    0xc3,                                        // ret

    // int3 filler
    0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc
};
