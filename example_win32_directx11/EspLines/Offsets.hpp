#pragma once
#ifndef OFFSETS_HPP
#define OFFSETS_HPP

class Offsets {
public:
    static inline uintptr_t Il2Cpp = 0x0;
    static inline uintptr_t InitBase = 0xA986E9C;
    static inline uintptr_t StaticClass = 0x5C;
    static inline uintptr_t CurrentMatch = 0x50;
    static inline uintptr_t MatchStatus = 0x8c;
    static inline uintptr_t LocalPlayer = 0x94;
    static inline uintptr_t DictionaryEntities = 0x68;
    static inline uintptr_t CurrentMatchGame = 0x4;

    static inline uintptr_t Player_IsDead = 0x50;
    static inline uintptr_t Player_Name = 0x2DC;
    static inline uintptr_t Player_Data = 0x48;
    static inline uintptr_t PlayerID = 0x2A0; // <BADPEFKDCED>k__BackingField - backing field de get_PlayerID (struct IHAAMHPPLMG_o, primer uint32)
    static inline uintptr_t AccountID = 0x2A4; // <BADPEFKDCED>k__BackingField - backing field de get_AccountID (struct IHAAMHPPLMG_o, segundo uint32)
    static inline uintptr_t BaseProfileInfo = 0x18CC;

    static inline uintptr_t Player_ShadowBase = 0x18B8;
    static inline uintptr_t XPose = 0x78;
    static inline uintptr_t IsClientBot = 0x2E4;

    static inline uintptr_t AvatarManager = 0x4C0;
    static inline uintptr_t Avatar = 0xA8;
    static inline uintptr_t Avatar_IsVisible = 0x95;
    static inline uintptr_t Avatar_Data = 0x14;
    static inline uintptr_t Avatar_Data_IsTeam = 0x59;
    static inline uintptr_t Avatar_Data_IsBot = 0x2E4;

    static inline uintptr_t FollowCamera = 0x450;
    static inline uintptr_t Camera = 0x18;
    static inline uintptr_t AimRotation = 0x400;
    static inline uintptr_t AimRotationCheck = 0x3B8;

    static inline uintptr_t CurrentObserver = 0xB4;
    static inline uintptr_t ObserverPlayer = 0x28;

    static inline uintptr_t MainCameraTransform = 0x24C;

    static inline uintptr_t Weapon = 0x3F4;
    static inline uintptr_t WeaponData = 0x58;
    static inline uintptr_t WeaponRecoil = 0x0C;
    static inline uintptr_t UnkPlayerWeaponInfoClass = 0x4A8;
    static inline uintptr_t IsCombineWeapon = 0xD8;
    static inline uintptr_t WeaponOnHand = 0x54;
    static inline uintptr_t CombineWeaponOnHand = 0x58;
    static inline uintptr_t WeaponInfo = 0x64;
    static inline uintptr_t WeaponID = 0x14;
    static inline uintptr_t WeaponParams = 0x6C;
    static inline uintptr_t WeaponParams_FireInterval = 0x1C;
    static inline uintptr_t WeaponParams_RepeatFireInterval = 0x24;
    static inline uintptr_t WeaponParams_MultiFireInterval = 0x28;
    static inline uintptr_t Weapon_AddFireSpeed = 0x45C;
    static inline uintptr_t ActiveUISightingWeapon = 0x3F4;
    static inline uintptr_t AmmoOffs = 0x3F8; // MMPLDMPMBMO (dump actual, clase Weapon FDAEPHMIEPC)
    static inline uintptr_t FireComponent = 0x58;
    static inline uintptr_t tangentTheta = 0xC;

    static inline uintptr_t IsFiring = 0x540;

    static inline uintptr_t LockedAimingCollider = 0x54;
    static inline uintptr_t HeadCollider = 0x4A4;
    static inline uintptr_t NeckCollider = 0x454; // TODO: Find offset
    static inline uintptr_t ChestCollider = 0x45C; // TODO: Find offset

    static inline uintptr_t ViewMatrix = 0xE8;
    static inline uintptr_t Vida = 0x10;

    static inline uintptr_t IsGirl = 0x7C1;
    static inline uintptr_t ListTransform = 0x754;

    static inline uintptr_t PlayerAttributes = 0x4BC;
    static inline uintptr_t LocalPlayerAttributes = 0x4BC;
    static inline uintptr_t MedkitHackOffset = 0x60;

    static inline uintptr_t PlayerAttributes_FireIntervalScale = 0x18C;
    static inline uintptr_t PlayerAttributes_FireIntervalScaleTwo = 0x19C;
    static inline uintptr_t FallingSpeedUpScale = 0x1D4;
    static inline uintptr_t BuffWeaponAmmoClip = 0xD0;

    static inline uintptr_t NoReload = 0x99;
    static inline uintptr_t RunSpeedUpScale = 0x1D8; // velocidad de carrera (speed hack)
    static inline uintptr_t NoReload2 = 0x99; // alias de NoReload para WeaponAttributes (LV4)
    static inline uintptr_t GameTimer = 0x10;
    static inline uintptr_t FixedDeltaTime = 0x24;
    static inline uintptr_t Profile_Rank = 0x58;
    static inline uintptr_t Profile_RankPoint = 0x5C;
    static inline uintptr_t Pool_Health = 0x10;
    static inline uintptr_t FastSwitch = 0x51C;

    static inline uintptr_t tiro = 0x2C;
    static inline uintptr_t telepneu = 0x0;
    //static inline uintptr_t GhostMode = 0x38; comentado hasta encontrar una solcion 
    static inline uintptr_t PhysxData = 0x139C;

    // === SILENT AIM ===
    static inline uintptr_t LastAimingInfoFromWeapon = 0x978;   // puntero a la info de apuntado (localPlayer)
    static inline uintptr_t StartPosition = 0x38;               // posicion de salida del proyectil (aimingInfo)
    static inline uintptr_t RayDir = 0x2C;                      // direccion del rayo a escribir (aimingInfo)

    class Bones {
    public:
        static inline uintptr_t Head = 0x458;
        static inline uintptr_t Spine = 0x460;
        static inline uintptr_t Neck = 0x460;   // m_NeckNode (OB54 V7A: no hay bone 0x454)
        static inline uintptr_t Hip = 0x45C;
        static inline uintptr_t Root = 0x46C;
        static inline uintptr_t LeftAnkle = 0x474;
        static inline uintptr_t RightAnkle = 0x478;
        static inline uintptr_t LeftFoot = 0x47C;
        static inline uintptr_t RightFoot = 0x480;
        static inline uintptr_t RightShoulder = 0x490;
        static inline uintptr_t LeftShoulder = 0x48C;
        static inline uintptr_t LeftHand = 0x498;
        static inline uintptr_t RightHand = 0x494;
        static inline uintptr_t RightElbow = 0x49C;
        static inline uintptr_t LeftElbow = 0x4A0;
        static inline uintptr_t RightWrist = 0x484;
        static inline uintptr_t LeftWrist = 0x488;
        static inline uintptr_t Pelvis = 0x45C;
        static inline uintptr_t Hip2 = 0x46C;
        static inline uintptr_t HipCenter = 0x45C;
        static inline uintptr_t ToeRight = 0x484;
        static inline uintptr_t ToeLeft = 0x488;
        // OB54 V7A: no existen bones de rodilla (0x470 no es valido)
        static inline uintptr_t LeftKnee = 0x0;
        static inline uintptr_t RightKnee = 0x0;
        static inline uintptr_t LeftToe = 0x480;
        static inline uintptr_t RightToe = 0x484;
        // Skeleton offsets (user version)
        static inline uintptr_t S_LeftWrist = 0x3F4;
        static inline uintptr_t S_RightWrist = 0x420;
        static inline uintptr_t S_LeftWristJoint = 0x438;
        static inline uintptr_t S_RightWristJoint = 0x434;
    };
};

#endif // OFFSETS_HPP