#pragma once
// tes_types.h — TES-0D-02-01 應用層共用型別
//
// 匯流排上的東西（6 個 CAN 訊框結構、位元定義、tes_state_t）已移到 tes_wire.h，
// 本檔只保留應用層型別。只需要協定本身的一端請改 include tes_wire.h，就不會把另一
// 端的應用型別拉進自己的編譯單元。
//
// 零平台依賴：只用 stdint.h / stdbool.h，可在任何 C99 環境編譯
#include <stdint.h>
#include <stdbool.h>
#include "tes_protocol/tes_wire.h"


// ─── 充電停止模式 ──────────────────────────────────────────────────────────────

typedef enum {
    STOP_MODE_SOC     = 0,   // 依 BMS 回報 SOC 停止（預設）
    STOP_MODE_VOLTAGE = 1,   // 依輸出電壓停止
    STOP_MODE_TIMER   = 2,   // 依充電時長停止
} stop_mode_t;

// ─── CP 電壓狀態（以 ADS1115 讀取直流電壓判斷） ──────────────────────────────

typedef enum {
    CP_STATE_UNKNOWN = 0,
    CP_STATE_OFF,       // 0.0 – 1.9 V
    CP_STATE_ON,        // 7.4 – 13.7 V
    CP_STATE_ERROR,     // 其他電壓值
} cp_state_t;

// ─── LED 指示燈狀態 ────────────────────────────────────────────────────────────

typedef enum {
    LED_STATE_STANDBY = 0,
    LED_STATE_CHARGING,
    LED_STATE_COMPLETE,
    LED_STATE_FAULT,
} led_state_t;

// ─── 預充電子步驟（供單元測試可見） ──────────────────────────────────────────

typedef enum {
    PRECHARGE_STEP_INIT = 0,
    PRECHARGE_STEP_READY_ANNOUNCED,  // 已發送 0x508 ready 位元
    PRECHARGE_STEP_CONTACTOR_WAIT,   // 等待車端接觸器閉合
    PRECHARGE_STEP_RELAY_DELAY,      // 接觸器閉合後 250ms 延遲
    PRECHARGE_STEP_COMPLETE,         // 充電繼電器已閉合，進入 CHARGING
} precharge_step_t;


// ─── 狀態機配置（初始化時傳入，運行中不變） ──────────────────────────────────

typedef struct {
    uint16_t max_voltage_01v;       // 硬體上限，如 1000 = 100.0V
    uint16_t max_current_01a;       // 硬體上限，如 100 = 10.0A
    int8_t   target_soc;            // 目標電量 0-100%
    uint16_t manufacturer_code;     // H'5F8 製造商代碼
} tes_sm_config_t;

// ─── 充電停止原因 ──────────────────────────────────────────────────────────────

typedef enum {
    STOP_REASON_NORMAL  = 0,  // SOC 目標達成
    STOP_REASON_USER    = 1,  // 手動停止（按鈕或 REST API）
    STOP_REASON_FAULT   = 2,
    STOP_REASON_EMERG   = 3,
    STOP_REASON_BMS     = 4,  // BMS 撤回充電許可
    STOP_REASON_TIMER   = 5,  // 計時到達
    STOP_REASON_VOLTAGE = 6,  // 電壓目標達成
} stop_reason_t;

// ─── 故障來源（僅供診斷／顯示，不透過 CAN 傳送）──────────────────────────────
//
// status_508.fault_flags 的位元由 TES-0D-02-01 定義且會送往車端 BMS，
// 因此不可用來編碼自訂原因。此欄位是純內部診斷值。

// 每個項目後面標註 fault_ctx_a / fault_ctx_b 的意義。
typedef enum {
    FAULT_SRC_NONE = 0,
    FAULT_SRC_VEHICLE_TIMEOUT,      // PARAM_EXCHANGE 等待 CAN 許可逾時（15s）
                                    //   a = 等待秒數  b = 最後看到的 0x500 status_flags
    FAULT_SRC_VOLTAGE_INCOMPAT,     // 車端要求電壓 > 使用者設定的 Max Voltage
                                    //   a = 車端要求 0.1V  b = 使用者設定上限 0.1V
    FAULT_SRC_PRECHARGE_TIMEOUT,    // PRE_CHARGE 等待 CP + CAN 許可逾時（20s）
                                    //   a = CP 電壓 0.1V  b = 0x500 status_flags
    FAULT_SRC_CONTACTOR_TIMEOUT,    // 等待車端接觸器閉合逾時（10s）
                                    //   a = 等待秒數  b = 0x500 status_flags
    FAULT_SRC_PSU_LOST,             // 充電中 PSU 斷線（session 起始時本來有連線）
                                    //   a = 已充電秒數  b = 0
    FAULT_SRC_BMS_FAULT,            // 車端 0x500 fault_flags != 0
                                    //   a = fault_flags  b = status_flags
    FAULT_SRC_CP_LOST,              // 充電中 CP 斷開（手動模式）
                                    //   a = CP 電壓 0.1V  b = cp_state_t
    FAULT_SRC_EMERGENCY_BTN,        // 硬體緊急停止按鈕            a = b = 0
    FAULT_SRC_EMERGENCY_VEHICLE,    // 車端 0x5F0 緊急停止請求
                                    //   a = 0x5F0 error_request_flags  b = 0
    FAULT_SRC_OVER_CURRENT,         // 實測電流持續超過車端請求或最大電流（電源無法被命令，例如
                                    //   旋鈕電源：BMS 降低請求時電源照推）
                                    //   a = 實測 0.1A  b = 車端請求 0.1A
} fault_source_t;

// ─── 充電紀錄（可放入 charger_event_t payload，24 bytes = 上限） ──────────────

typedef struct {
    uint32_t duration_s;       // 充電時長（秒）
    float    energy_wh;        // 充電電量（Wh，V×I × 10ms / 3600000）
    float    stop_voltage_v;   // 停止時輸出電壓（STOP_REASON_VOLTAGE 時有效，其他為 0）
    uint32_t session_id;       // 全域遞增序號（NVS 鍵 sess_seq），用來對應 trace_svc 的曲線／Log
    uint8_t  soc_start;        // 充電開始時 SOC（0-100）
    uint8_t  soc_end;          // 充電結束時 SOC（0-100）
    uint8_t  stop_reason;      // stop_reason_t
    uint8_t  energy_estimated; // 1 = PSU 未連線，電量為 ADC 預估值
    // stop_reason=STOP_REASON_FAULT/EMERG 時只知道「故障了」，對使用者毫無幫助。
    // 這兩欄把當下的診斷資訊一起存進歷史，讓紀錄自己就能說明原因，
    // 不必叫使用者去翻 Log。意義與 tes_snapshot_t 的同名欄位一致。
    uint8_t  fault_source;     // fault_source_t，非故障停止時為 FAULT_SRC_NONE
    uint8_t  _pad;             // 對齊 fault_ctx_a
    uint16_t fault_ctx_a;      // 故障情境值，意義依 fault_source 而定
} charge_session_t;            // 24 bytes（正好等於 charger_event_t.payload 上限）

// ─── CAN 診斷快照（最後收到的 0x500 / 0x501 解碼值） ────────────────────────

// TWAI 控制器狀態（can_driver_get_health()）
typedef enum {
    CAN_BUS_STOPPED = 0,
    CAN_BUS_RUNNING,
    CAN_BUS_OFF,
    CAN_BUS_RECOVERING,
} can_bus_state_t;

typedef struct {
    // ── 0x500  Vehicle → Charger ──────────────────────────────────────────
    uint8_t  v500_fault;        // byte 0：故障旗標，見 V500_FAULT_*
    uint8_t  v500_status;       // byte 1：狀態表示旗標，見 V500_ST_*
    float    v500_req_current;  // bytes 2-3：充電電流命令值 (A)
    float    v500_req_voltage;  // bytes 4-5：充電電壓上限值 (V)
    float    v500_max_voltage;  // bytes 6-7：最大充電電壓 (V)
    // ── 0x501  Vehicle → Charger ──────────────────────────────────────────
    uint8_t  v501_seq;          // byte 0：esChargeSequenceNumber
    uint8_t  v501_soc;          // byte 1：SOC (%)
    uint16_t v501_max_time;     // bytes 2-3：最大充電時間 (min)，0xFFFF=未知
    uint16_t v501_eta;          // bytes 4-5：預估結束時間 (min)，0xFFFF=未知
    // ── 0x5F0  Vehicle → Charger（緊急）────────────────────────────────────
    uint8_t  v5f0_flags;        // byte 0：bit0=緊急停止請求, bit1=熔接異常
    float    v5f0_max_current;  // bytes 2-3：本次充電最大充電電流 (A)
    uint16_t v5f0_maker;        // bytes 4-5：車輛製造商認證編號
    // ── 0x508  Charger → Vehicle ──────────────────────────────────────────
    uint8_t  c508_fault;        // byte 0：故障旗標，見 C508_FAULT_*
    uint8_t  c508_status;       // byte 1：狀態表示旗標，見 C508_ST_*
    float    c508_avail_voltage;// bytes 2-3：可用輸出電壓 VLIM (V)
    float    c508_avail_current;// bytes 4-5：可用輸出電流 I_MX (A)
    float    c508_fault_voltage;// bytes 6-7：異常判定電壓上限 VLIM2 (V)
    // ── 0x509  Charger → Vehicle ──────────────────────────────────────────
    uint8_t  c509_seq;          // byte 0：esChargeSequenceNumber
    uint8_t  c509_rated_kw;     // byte 1：額定功率（×50 W）
    float    c509_voltage;      // bytes 2-3：實際輸出電壓 (V)
    float    c509_current;      // bytes 4-5：實際輸出電流 (A)
    uint16_t c509_remaining;    // bytes 6-7：剩餘時間 (min)，0xFFFF=未知
    // ── 0x5F8  Charger → Vehicle（緊急）────────────────────────────────────
    uint8_t  c5f8_flags;        // byte 0：bit0=緊急停止
    uint16_t c5f8_maker;        // bytes 4-5：製造商代碼

    // ── 收發活性 ──────────────────────────────────────────────────────────
    // 沒有這組數字就分不出「車端沒送」和「車端送了但值是 0」，
    // 而這正是 CAN 接不上時最先要判斷的事。
    uint32_t rx_500_count, rx_501_count, rx_5f0_count;
    uint32_t rx_500_age_ms, rx_501_age_ms, rx_5f0_age_ms;  // 0xFFFFFFFF = 從未收到
    uint32_t tx_508_count, tx_509_count, tx_5f8_count;
    uint32_t tx_fail_count;     // twai_transmit() 失敗次數（佇列滿／bus-off）

    // ── TWAI 控制器健康度 ────────────────────────────────────────────────
    uint8_t  bus_state;         // can_bus_state_t
    uint32_t bus_tx_err;        // TX error counter（≥128 進入 error passive）
    uint32_t bus_rx_err;        // RX error counter
    uint32_t bus_arb_lost;      // 仲裁失敗次數
    uint32_t bus_err_count;     // bus error（位元／格式錯誤）累計，接線不良會一直漲
    uint32_t bus_rx_missed;     // driver 佇列滿而漏掉的幀
} tes_can_diag_t;

// ─── 快照（只讀，供 display / network / 未來 JS 讀取） ─────────────────────

typedef struct {
    tes_state_t   state;
    bool          fault_latched;
    bool          charge_complete;
    uint8_t       soc;                   // 0-100 %
    float         output_voltage;        // V（PSU 回報或 ADC 量測）
    float         output_current;        // A
    float         vehicle_req_voltage;   // V（BMS 充電電壓上限，0x500 charge_voltage_limit）
    float         vehicle_req_current;   // A（BMS 請求電流，0x500 charge_current_cmd）
    bool          timer_running;
    uint32_t      elapsed_seconds;
    uint32_t      total_seconds;
    uint32_t      remaining_seconds;
    uint16_t      max_voltage_01v;       // 當前設定值
    uint16_t      max_current_01a;
    int8_t        target_soc;
    led_state_t   led_state;
    uint8_t       last_fault_flags;      // 最後一次故障旗標（用於顯示）
    uint8_t       fault_source;          // fault_source_t，本次故障的內部原因（診斷用）
    // 故障當下的關鍵數值，意義依 fault_source 而定（見 fault_source_t 註解）。
    // 狀態機只負責留下數字，文字敘述由顯示層組合 —— tes_protocol 不做字串格式化。
    uint16_t      fault_ctx_a;
    uint16_t      fault_ctx_b;
    uint8_t       stop_reason;           // stop_reason_t，最後一次充電結束的原因
    uint8_t       cp_state;              // cp_state_t，CP 腳位判定狀態（診斷／Log 用）
    float         last_valid_req_current;
    float         energy_wh;            // 本次充電累積電量（充電中有效，其他狀態為 0）
    bool          psu_connected;        // PSU UART 已回報（false = 數值為預估）
    tes_can_diag_t can;                 // 最後收到的 CAN 解碼值（診斷用）
} tes_snapshot_t;
