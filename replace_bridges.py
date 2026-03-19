import os
import re

client_bridge_files = [
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Shared\GamePlay\Events\BaseEvent.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Shared\Character\MyCharacter.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Server\GamePlay\Events\PersonalEvents\PersonalEventBizarreForge.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Server\GamePlay\Events\PersonalEvents\PersonalEventCursedSword.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Server\GamePlay\Events\GroupEvents\GroupEventConfiscate.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Server\GamePlay\Events\PersonalEvents\PersonalEventInspectMalice.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Server\GamePlay\Events\PersonalEvents\PersonalEventMaliceOverload.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Server\GamePlay\Events\PersonalEvents\PersonalEventPublicMalice.cpp"
]

for f in client_bridge_files:
    if not os.path.exists(f): continue
    with open(f, 'r', encoding='utf-8') as file:
        content = file.read()
    content = content.replace('#include "Interface/A302ClientEventBridge.h"', '#include "Character/MyPlayerController.h"')
    content = content.replace('IA302ClientEventBridge', 'AMyPlayerController')
    with open(f, 'w', encoding='utf-8') as file:
        file.write(content)

char_bridge_files = [
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Shared\GamePlay\Items\ItemTimeKnife.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Shared\GamePlay\Items\ItemShield.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Shared\GamePlay\Items\ItemKnife.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Shared\Character\Dummy\DummyCharacter.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Shared\Character\Components\InteractComponent.h",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Shared\Character\Components\InteractComponent.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Client\Animation\AnimNotify\HideWeapon.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Client\Animation\AnimNotify\ShowWeapon.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Server\GamePlay\Events\PersonalEvents\PersonalEventCursedSword.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Server\GamePlay\Events\PersonalEvents\PersonalEventPublicMalice.cpp",
    r"c:\Users\SSAFY\Desktop\2div\S14P21A302\A302\Source\A302Server\GamePlay\Events\PersonalEvents\PersonalEventMaliceOverload.cpp"
]

for f in char_bridge_files:
    if not os.path.exists(f): continue
    with open(f, 'r', encoding='utf-8') as file:
        content = file.read()
    content = content.replace('#include "Interface/A302CharacterBridge.h"', '#include "Character/MyCharacter.h"')
    content = content.replace('IA302CharacterBridge', 'AMyCharacter')
    with open(f, 'w', encoding='utf-8') as file:
        file.write(content)
print("done")
