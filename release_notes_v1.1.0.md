### 🚀 Overdrive Core v1.1.0 (Garage Assembly, 3D Weapons, AI Bot & Procedural Motion)

解凍してすぐに遊べるWindows 64bit向けビルド済みスタンドアローンパッケージです。
DirectX 11 + SDL3 + Jolt Physics + fastgltf + OpenXR PCVR 完全統合版。

#### 🎮 主な新機能・アップデート内容 (v1.1.0)
1. **ガレージ・アセンブル機能 (Garage / Assembly Mode)**
   - 全4部位・16種類の機体フレームパーツ換装（HEAD, CORE, ARMS, LEGS）
   - 全3系統・12種類の内装パーツ換装（BOOSTER, FCS, GENERATOR）
   - 機体スペック（AP、重量、EN容量、巡航速度、QB推力、跳躍力等）のリアルタイム動的再計算
   - 機体構成の保存・復元（`loadout.ini` 自動セーブ）
2. **fastgltf による glTF 2.0 / GLB 3D武器モデル**
   - 全9兵装（ライフル、ビーム、プラズマ、バーストガン、誘導ミサイル、バズーカ、ブレード、ガトリング、ショットガン）の3Dモデルを搭載
3. **肩部ミサイル直接斉射 & ウェポンハンガー分離**
   - 肩部ミサイル（`Q` / `E` / `LB` / `RB`）は肩から直接射出、その他の手持ち腕武器は瞬時スワップ
4. **自律戦闘 AI ボットメカ (AI Bot Mech)**
   - トレーニングモードでプレイヤーと超高速立体機動で交戦する自律戦闘AIボットを配備
5. **多層発光スラスタージェット & プロシージャルメカモーション**
   - 状態別（ホバー巡航、QB、AB、垂直上昇）のダイナミック3Dプラズマ炎噴射
   - 射撃時リコイル反動、照準仰角サーボ追従、ガトリングスピン、着地サスペンション
6. **OpenXR PCVR 完全対応**
   - Meta Quest 3S / PCVR HMD による一人称コックピット立体視 (FPV) & 3DキャノピーHUD
7. **ゲーム内完結 Tailscale マルチプレイ & DIRECT IP CONNECT 画面**
   - 外部バッチやini編集不要！ゲーム内GUIだけでTailscale対戦に参加・ホスト可能
   - **Tailscale IP自動検出**: PC上のTailscale IPv4（`100.x.x.x`）を自動検出し、画面上にワンキーコピー（`[C]` キー）付きで表示
   - **ワンタッチペースト**: 友達のIPを `Ctrl+V` で即座に貼り付け＆入力IPの自動保存（次回ワンクリック再接続）
   - ゲームパッド・VR両眼立体コックピット前面ホログラムUIにも完全対応


#### 📦 同梱内容
- `OverdriveCore.exe`: ゲーム本体 (Release最適化版)
- `SDL3.dll`: SDL3ランタイム
- `assets/`: シェーダー、3Dモデル（GLB）、3D空間音響（WAV）
- `loadout.ini`: 機体アセンブル保存設定ファイル
- `network.ini`: マルチプレイヤー設定ファイル
- `MULTIPLAYER_GUIDE.md`: 対戦ガイド
- `ゲーム起動.bat`: 通常起動
- `ホストとして起動.bat`: マルチプレイヤーホスト待機
- `クライアントとして接続.bat`: IPを入力して接続
