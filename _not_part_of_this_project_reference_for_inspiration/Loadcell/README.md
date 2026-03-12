# ESP32-S3 + HX711 + Loadcell

Hieronder staat de toelichting voor het gebruik van de Arduino-sketch voor jouw ESP32-S3 met HX711 en loadcell.

## Benodigde library

Je hebt hiervoor één extra library nodig:

- **HX711** van Bogde

Links:

- GitHub: https://github.com/bogde/HX711
- Arduino Libraries: https://www.arduinolibraries.info/libraries/hx711

## Opmerking over de code

De sketch zet **IO6 meteen HIGH** en **IO9 meteen LOW** in `setup()`, zodat jouw HX711 via die twee pinnen van voeding wordt voorzien:

- **IO6** = voeding voor HX711
- **IO9** = ground voor HX711
- **IO7** = SCK
- **IO8** = DT / DOUT

Let op: software kan die pinnen pas zetten zodra de sketch start. Daarom gebeurt dat zo vroeg mogelijk in `setup()`.

## Gebruik

1. Installeer de library **HX711**.
2. Upload de sketch.
3. Open de **Serial Monitor** op **115200 baud**.
4. Zet **line ending** op **Newline** of **Both NL & CR**.
5. Typ eerst `tare` terwijl de loadcell leeg is.
6. Kalibreren doe je bijvoorbeeld met:

```text
cal 500
```

als je een gewicht van **500 gram** hebt.

7. Daarna kun je wegen met:

```text
weight
```

of continu meten met:

```text
monitor
```

## Beschikbare commando's

- `help` — toon hulp
- `status` — toon status
- `tare` — zet huidige lege waarde op 0
- `raw` — toon ruwe HX711 waarde
- `weight` — toon gewicht in gram
- `monitor` — gewicht continu tonen aan/uit
- `factor` — toon huidige kalibratiefactor
- `cal 500` — kalibreer met bekend gewicht, bijvoorbeeld 500 gram
- `clearcal` — wis opgeslagen kalibratiefactor

## Praktische punten

- De code slaat de **kalibratiefactor** op, maar niet de actuele nulstand.
  Na een reboot is het dus slim om nog even `tare` te doen terwijl de weegschaal leeg is.
- Als je meting negatief loopt terwijl het gewicht toeneemt, is dat niet erg.
  Dan wordt de kalibratiefactor vanzelf negatief en werkt het alsnog.

## Tip

Als de HX711 niet reageert:

- controleer de bedrading
- controleer of IO6 echt HIGH wordt
- controleer of IO9 echt LOW wordt
- controleer of de HX711 voldoende voeding krijgt
