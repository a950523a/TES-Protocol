#pragma once
// tes_wire.h — TES-0D-02-01 匯流排上的東西，僅此而已
//
// 這裡只放「線上真的會跑的」：6 個 CAN 訊框結構、其位元定義，與雙方共用的狀態
// 列舉。任一端專屬的應用型別（顯示、PSU、充電紀錄、故障來源、次流程步驟…）留在
// tes_types.h，兩端各自只取自己需要的那一層。
//
// 零平台依賴：只用 stdint.h / stdbool.h，可在任何 C99 環境編譯
#include <stdint.h>
#include <stdbool.h>

// ─── 狀態機狀態 ───────────────────────────────────────────────────────────────

typedef enum {
    TES_STATE_IDLE = 0,
    TES_STATE_PARAM_EXCHANGE,   // 初始參數交換（等待 CAN 許可）
    TES_STATE_PRE_CHARGE,       // 預充電操作（電磁鎖、絕緣測試、繼電器）
    TES_STATE_CHARGING,         // DC 電流輸出中
    TES_STATE_ENDING,           // 正常結束流程（斷繼電器、等車端）
    TES_STATE_FAULT,            // 故障處理（顯示 10 秒後回 IDLE）
    TES_STATE_EMERGENCY,        // 緊急停止程序
    TES_STATE_FINALIZE,         // 最終化（確認輸出電壓歸零）
} tes_state_t;
// ─── CAN 訊框結構（TES-0D-02-01）────────────────────────────────────────────

// ─── H'500 byte 0：故障旗標（TES-0D-02-01 表 16）──────────────────────────────
// 全部都是 0=正常、1=異常。bit 6-7 預備，固定 0。
#define V500_FAULT_SUPPLY_SYSTEM   0x01u  // bit0 供電系統異常檢知
                                          //   ⚠ 這是車輛在指控「充電樁」異常，
                                          //     不是電池本身的問題
#define V500_FAULT_BATT_OVERVOLT   0x02u  // bit1 電池過電壓檢知
#define V500_FAULT_BATT_UNDERVOLT  0x04u  // bit2 電池不足電壓異常檢知
#define V500_FAULT_CURRENT_DIFF    0x08u  // bit3 電池電流差異異常檢知
#define V500_FAULT_BATT_OVERTEMP   0x10u  // bit4 電池高溫異常檢知
#define V500_FAULT_VOLT_DIFF       0x20u  // bit5 電池電壓差異常檢知

// ─── H'500 byte 1：狀態表示旗標（TES-0D-02-01 表 16）─────────────────────────
// 注意各位元的極性不一致，不要一律當成「1=有問題」。bit 4-7 預備，固定 0。
#define V500_ST_CHARGE_PERMIT      0x01u  // bit0 可進行車輛充電   0=不可, 1=可
#define V500_ST_CONTACTOR_OPEN     0x02u  // bit1 車輛狀態
                                          //   0=接觸器關閉／熔接診斷中
                                          //   1=接觸器斷開／熔接診斷終了
#define V500_ST_POSTURE_NG         0x04u  // bit2 車輛充電姿勢     0=可充電, 1=不可充電
#define V500_ST_NORMAL_STOP_REQ    0x08u  // bit3 充電前正常停止要求 0=無, 1=有

// H'500  Vehicle → Charger（狀態與需求）
typedef struct {
    uint8_t  fault_flags;           // byte 0：故障旗標，見 V500_FAULT_*
    uint8_t  status_flags;          // byte 1：狀態表示旗標，見 V500_ST_*
    uint16_t charge_current_cmd;    // bytes 2-3：0.1A/bit，充電電流命令值
    uint16_t charge_voltage_limit;  // bytes 4-5：0.1V/bit，充電電壓上限值
    uint16_t max_charge_voltage;    // bytes 6-7：0.1V/bit，最大充電電壓
} tes_vehicle_status_t;             // CAN ID 0x500

// H'501  Vehicle → Charger（參數）
typedef struct {
    uint8_t  seq_num;               // byte 0：esChargeSequenceNumber
    uint8_t  soc;                   // byte 1：1%/bit，電量百分比
    uint16_t max_charge_time_min;   // bytes 2-3：1min/bit，0xFFFF=未知
    uint16_t est_end_time_min;      // bytes 4-5：1min/bit，預估結束時間
} tes_vehicle_params_t;             // CAN ID 0x501

// H'5F0  Vehicle → Charger（緊急）
typedef struct {
    uint8_t  error_request_flags;      // byte 0：bit0=緊急停止請求 bit1=熔接異常
    uint16_t max_charge_current_01a;   // bytes 2-3：本次充電最大充電電流，0.1A/bit
    uint16_t manufacturer_id;          // bytes 4-5：車輛製造商認證編號
} tes_vehicle_emergency_t;             // CAN ID 0x5F0

// ─── H'508 byte 0：故障旗標（TES-0D-02-01 表 16）──────────────────────────────
// bit 3-7 預備，固定 0。這是我們主動送給車端 BMS 的，位元用錯等於謊報故障類型。
#define C508_FAULT_SUPPLY_SYSTEM   0x01u  // bit0 供電系統異常      0=正常, 1=發生
#define C508_FAULT_DEVICE_ABNORMAL 0x02u  // bit1 直流供電裝置異常  0=正常, 1=異常
                                          //   「直流供電裝置」＝充電樁本體（含 PSU）
#define C508_FAULT_BATTERY_UNSUIT  0x04u  // bit2 電池不適合        0=適合, 1=不適合

// ─── H'508 byte 1：狀態表示旗標（TES-0D-02-01 表 16）─────────────────────────
// bit 3-7 預備，固定 0。注意 bit0 是「停止控制」不是「待機」，極性容易搞反。
#define C508_ST_STOP_CONTROL       0x01u  // bit0 停止控制
                                          //   0=輸出追隨運轉中, 1=停止控制中或停止狀態
#define C508_ST_CHARGING           0x02u  // bit1 裝置狀態  0=待機中, 1=充電中
#define C508_ST_COUPLER_LOCKED     0x04u  // bit2 電子鎖    0=解除,   1=閉鎖中

// H'508  Charger → Vehicle（充電樁狀態）
typedef struct {
    uint8_t  fault_flags;           // byte 0：故障旗標，見 C508_FAULT_*
    uint8_t  status_flags;          // byte 1：狀態表示旗標，見 C508_ST_*
    uint16_t available_voltage;     // bytes 2-3：可用輸出電壓（VLIM），0.1V/bit
    uint16_t available_current;     // bytes 4-5：可用輸出電流（I_MX），0.1A/bit
    uint16_t fault_detect_voltage;  // bytes 6-7：異常判定電壓上限值（VLIM2），0.1V/bit
} tes_charger_status_t;             // CAN ID 0x508

// H'509  Charger → Vehicle（充電樁參數）
typedef struct {
    uint8_t  seq_num;               // byte 0：esChargeSequenceNumber（硬編碼 18）
    uint8_t  rated_power_50w;       // byte 1：50W/bit
    uint16_t actual_voltage;        // bytes 2-3：0.1V/bit，實際輸出電壓
    uint16_t actual_current;        // bytes 4-5：0.1A/bit，實際輸出電流
    uint16_t remaining_time_min;    // bytes 6-7：1min/bit，0xFFFF=未知
} tes_charger_params_t;             // CAN ID 0x509

// H'5F8  Charger → Vehicle（緊急）
typedef struct {
    uint8_t  emergency_flags;       // byte 0：bit0=緊急停止
    uint16_t manufacturer_id;       // bytes 4-5：製造商代碼
} tes_charger_emergency_t;          // CAN ID 0x5F8
