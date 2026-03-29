// by Marius Versteegen, 2025
// CSS for the Grid visualization page, served separately to reduce HTML size.

#pragma once

namespace crt
{
	const char GRID_CSS[] = R"rawliteral(
    body {
      margin: 0;
      font-family: system-ui, sans-serif;
      background: #fafafa;
    }
    nav {
      background: #333;
      padding: 0.5rem 1rem;
      display: flex;
      gap: 1.5rem;
    }
    nav a {
      text-decoration: none;
      font-weight: 600;
    }
    .page {
      max-width: 1200px;
      margin: 1.5rem auto;
      padding: 1rem;
      background: #fff;
      border: 1px solid #ddd;
    }
    h1 {
      text-align: center;
      margin-bottom: 0.5rem;
    }
    .controls {
      margin-bottom: 1rem;
      display: flex;
      gap: 1rem;
      justify-content: center;
      align-items: flex-start;
    }
    .ctrl-buttons {
      display: flex;
      flex-wrap: wrap;
      gap: 0.4rem;
    }
    .ctrl-fields {
      display: flex;
      flex-direction: column;
      gap: 0.3rem;
      font-size: 0.75rem;
    }
    .ctrl-fields label {
      display: flex;
      align-items: center;
      gap: 0.3rem;
    }
    .ctrl-fields input {
      font-size: 0.75rem;
      padding: 0.15rem 0.3rem;
      border: 1px solid #999;
      border-radius: 3px;
      width: 70px;
    }
    .toggle-btn {
      padding: 0.4rem 1rem;
      border: 2px solid #999;
      border-radius: 4px;
      background: #fff;
      cursor: pointer;
      font-size: 0.85rem;
      font-weight: 600;
      color: #666;
    }
    .toggle-btn.active {
      background: #333;
      color: #fff;
      border-color: #333;
    }
    .sensor-layout {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr 1fr;
      gap: 0.5rem;
    }
    .sensor-widget {
      border: 1px solid #ddd;
      border-radius: 6px;
      padding: 0.4rem;
      background: #fafafa;
    }
    .sensor-widget h3 {
      text-align: center;
      margin: 0 0 0.3rem;
      font-size: 0.8rem;
    }
    .offset-btn {
      display: block;
      margin: 0 auto 0.3rem;
      font-size: 0.7rem;
      padding: 0.2rem 0.5rem;
    }
    .sensor-widget .no-data {
      text-align: center;
      color: #999;
      padding: 2rem 0;
    }
    .grid-container {
      display: flex;
      flex-direction: column;
      align-items: flex-start;
      gap: 0;
      margin: 0 auto;
      width: fit-content;
    }
    .row {
      display: flex;
      gap: 2px;
      margin-top: -1px;
    }
    .row:first-child {
      margin-top: 0;
    }
    .cell {
      width: 22px;
      height: 22px;
      border-radius: 50%;
      border: 1px solid #ccc;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 0.4rem;
      color: #888;
      transition: background 0.2s ease-out;
    }
    .histogram {
      max-width: 100%;
      margin: 0.3rem auto 0;
      display: flex;
      align-items: flex-end;
      gap: 1px;
      height: 40px;
    }
    .hist-bar {
      flex: 1;
      background: #555;
      border-radius: 1px 1px 0 0;
      transition: height 0.2s ease-out;
      min-height: 1px;
    }
    .hist-axis {
      margin: 2px auto 0;
      display: flex;
      justify-content: space-between;
      font-size: 0.55rem;
      color: #888;
    }
    .stats-table {
      margin: 0.3rem auto 0;
      border-collapse: collapse;
      width: 100%;
      font-size: 0.65rem;
    }
    .stats-table th, .stats-table td {
      border: 1px solid #ccc;
      padding: 0.15rem 0.3rem;
      text-align: center;
    }
    .stats-table th {
      background: #f0f0f0;
      font-weight: 600;
    }
    .surface-canvas {
      display: none;
      width: 100%;
      height: 200px;
      margin: 0 auto;
      border: 1px solid #ccc;
      background: #111;
    }
    .panel-buttons {
      display: flex;
      gap: 0.3rem;
      justify-content: center;
      margin-bottom: 0.3rem;
    }
    .running-plot {
      display: block;
      width: 100%;
      height: 120px;
      margin: 0.3rem auto 0;
      border: 1px solid #ccc;
      background: #111;
    }
    .loadcell-section {
      margin-top: 0.4rem;
      padding: 0.3rem;
      border: 1px solid #bcd;
      border-radius: 4px;
      background: #f0f5fa;
      font-size: 0.7rem;
      display: none;
    }
    .loadcell-section h4 {
      margin: 0 0 0.2rem;
      font-size: 0.75rem;
      text-align: center;
    }
    .lc-weight {
      text-align: center;
      font-size: 1.1rem;
      font-weight: 700;
      margin: 0.2rem 0;
    }
    .lc-row {
      display: flex;
      align-items: center;
      gap: 0.3rem;
      margin: 0.15rem 0;
      justify-content: center;
    }
    .lc-row label {
      font-size: 0.65rem;
    }
    .lc-row input {
      font-size: 0.65rem;
      padding: 0.1rem 0.2rem;
      border: 1px solid #999;
      border-radius: 3px;
      width: 60px;
    }
    .lc-row button {
      font-size: 0.65rem;
      padding: 0.15rem 0.4rem;
      cursor: pointer;
    }
    .sensor-widget.selected {
      border-color: #2a2;
      border-width: 2px;
    }
    .selected-info-panel {
      display: none;
      border: 2px solid #2a2;
      border-radius: 6px;
      padding: 0.5rem;
      margin-bottom: 1rem;
      background: #f0faf0;
      font-size: 0.75rem;
    }
    .selected-info-panel.visible {
      display: flex;
      gap: 1rem;
      align-items: center;
      justify-content: center;
    }
    .selected-info-panel label {
      display: flex;
      align-items: center;
      gap: 0.3rem;
    }
    .selected-info-panel input {
      font-size: 0.75rem;
      padding: 0.15rem 0.3rem;
      border: 1px solid #999;
      border-radius: 3px;
      width: 70px;
    }
    .cell.cell-selected {
      border: 2px solid #22f;
    }
    .cell.cell-captured {
      border: 2px solid #000;
    }
    .circle-info-panel {
      display: none;
      border: 2px solid #c22;
      border-radius: 6px;
      padding: 0.5rem;
      margin-bottom: 1rem;
      background: #faf0f0;
      font-size: 0.75rem;
    }
    .circle-info-panel.visible {
      display: block;
    }
    .circle-info-row {
      display: flex;
      gap: 0.5rem;
      align-items: center;
      justify-content: center;
      margin: 0.2rem 0;
    }
    .circle-info-row label {
      display: flex;
      align-items: center;
      gap: 0.2rem;
    }
    .circle-info-row input {
      font-size: 0.75rem;
      padding: 0.15rem 0.3rem;
      border: 1px solid #999;
      border-radius: 3px;
      width: 60px;
    }
    #status {
      margin-top: 1rem;
      text-align: center;
      color: #666;
    }
    .rec-play-panel {
      margin-bottom: 1rem;
      padding: 0.5rem;
      border: 2px solid #aaa;
      border-radius: 6px;
      background: #f5f5f5;
      display: flex;
      gap: 0.5rem;
      align-items: center;
      justify-content: center;
    }
    .rec-play-panel button {
      padding: 0.4rem 1rem;
      border: 2px solid #999;
      border-radius: 4px;
      background: #fff;
      cursor: pointer;
      font-size: 0.85rem;
      font-weight: 600;
      color: #666;
    }
    .rec-play-panel button:disabled {
      opacity: 0.4;
      cursor: default;
    }
    .rec-play-panel button.rec-active {
      background: #d32f2f;
      color: #fff;
      border-color: #b71c1c;
    }
    .rec-play-panel button.pause-active {
      background: #f57c00;
      color: #fff;
      border-color: #e65100;
    }
    .rec-play-panel button.play-active {
      background: #2e7d32;
      color: #fff;
      border-color: #1b5e20;
    }
    .rec-play-panel .rec-info {
      font-size: 0.75rem;
      color: #666;
      min-width: 120px;
    }
    .snapshot-panel {
      margin-bottom: 1rem;
      padding: 0.5rem;
      border: 2px solid #b39ddb;
      border-radius: 6px;
      background: #f3e5f5;
      display: flex;
      flex-direction: column;
      gap: 0.4rem;
    }
    .snapshot-row {
      display: flex;
      gap: 0.5rem;
      align-items: center;
      justify-content: center;
    }
    .snapshot-panel button {
      padding: 0.4rem 1rem;
      border: 2px solid #999;
      border-radius: 4px;
      background: #fff;
      cursor: pointer;
      font-size: 0.85rem;
      font-weight: 600;
      color: #666;
    }
    .snapshot-panel button:disabled {
      opacity: 0.4;
      cursor: default;
    }
    .snapshot-panel button.snap-replay-active {
      background: #7b1fa2;
      color: #fff;
      border-color: #4a148c;
    }
    .snapshot-panel .snap-info {
      font-size: 0.75rem;
      color: #666;
    }
)rawliteral";

} // end namespace crt
