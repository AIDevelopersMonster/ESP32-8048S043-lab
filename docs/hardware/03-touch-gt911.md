# GT911 touch

Initial validation path:

1. I2C scan on reported SDA/SCL;
2. identify GT911 address;
3. read raw coordinates;
4. map to LCD coordinates;
5. five-target test: TL/TR/CENTER/BL/BR;
6. verify orientation consistency.
