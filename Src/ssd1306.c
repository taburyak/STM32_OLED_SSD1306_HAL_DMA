/*
 * ssd1306.c
 *
 *  Created on: 14/04/2018
 *  Update on: 10/04/2019
 *      Author: Andriy Honcharenko
 *      version: 2
 */

/* CODE BEGIN Includes */
#include "ssd1306.h"
/* CODE END Includes */

/* CODE BEGIN Private defines */
// Screen object
static SSD1306_t SSD1306;
// Screenbuffer
static uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE];
// SSD1306 display geometry
SSD1306_Geometry display_geometry = SSD1306_GEOMETRY;
/* CODE END Private defines */

/* CODE BEGIN Private functions */
//
//  Send a byte to the command register and data
//
static void ssd1306_WriteCommand(uint8_t command);
static void ssd1306_WriteData(uint8_t* data, uint16_t size);
//
//  Get a width and height screen size
//
static const uint16_t width(void)	{ return SSD1306_WIDTH; };
static const uint16_t height(void)  { return SSD1306_HEIGHT; };
/* CODE END Private functions */

/* CODE BEGIN Public functions */
uint16_t ssd1306_GetWidth(void)
{
  return SSD1306_WIDTH;
}

uint16_t ssd1306_GetHeight(void)
{
  return SSD1306_HEIGHT;
}

SSD1306_COLOR ssd1306_GetColor(void)
{
	return SSD1306.Color;
}

void ssd1306_SetColor(SSD1306_COLOR color)
{
	SSD1306.Color = color;
}

//	Initialize the oled screen
uint8_t ssd1306_Init(void)
{
	/* Check if LCD connected to I2C */
	if (HAL_I2C_IsDeviceReady(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 5, 1000) != HAL_OK)
	{
		SSD1306.Initialized = 0;
		/* Return false */
		return 0;
	}

	// Wait for the screen to boot
	HAL_Delay(100);

	/* Init LCD */
	ssd1306_WriteCommand(DISPLAYOFF);
	ssd1306_WriteCommand(SETDISPLAYCLOCKDIV);
	ssd1306_WriteCommand(0xF0); // Increase speed of the display max ~96Hz
	ssd1306_WriteCommand(SETMULTIPLEX);
	ssd1306_WriteCommand(height() - 1);
	ssd1306_WriteCommand(SETDISPLAYOFFSET);
	ssd1306_WriteCommand(0x00);
	ssd1306_WriteCommand(SETSTARTLINE);
	ssd1306_WriteCommand(CHARGEPUMP);
	ssd1306_WriteCommand(0x14);
	ssd1306_WriteCommand(MEMORYMODE);
	ssd1306_WriteCommand(0x00);
	ssd1306_WriteCommand(SEGREMAP);
	ssd1306_WriteCommand(COMSCANINC);
	ssd1306_WriteCommand(SETCOMPINS);

	if (display_geometry == GEOMETRY_128_64)
	{
	  ssd1306_WriteCommand(0x12);
	}
	else if (display_geometry == GEOMETRY_128_32)
	{
	  ssd1306_WriteCommand(0x02);
	}

	ssd1306_WriteCommand(SETCONTRAST);

	if (display_geometry == GEOMETRY_128_64)
	{
	  ssd1306_WriteCommand(0xCF);
	}
	else if (display_geometry == GEOMETRY_128_32)
	{
	  ssd1306_WriteCommand(0x8F);
	}

	ssd1306_WriteCommand(SETPRECHARGE);
	ssd1306_WriteCommand(0xF1);
	ssd1306_WriteCommand(SETVCOMDETECT); //0xDB, (additionally needed to lower the contrast)
	ssd1306_WriteCommand(0x40);	        //0x40 default, to lower the contrast, put 0
	ssd1306_WriteCommand(DISPLAYALLON_RESUME);
	ssd1306_WriteCommand(NORMALDISPLAY);
	ssd1306_WriteCommand(0x2e);            // stop scroll
	ssd1306_WriteCommand(DISPLAYON);

	// Set default values for screen object
	SSD1306.CurrentX = 0;
	SSD1306.CurrentY = 0;
	SSD1306.Color = Black;

	// Clear screen
	ssd1306_Clear();

	// Flush buffer to screen
	ssd1306_UpdateScreen();

	SSD1306.Initialized = 1;

	/* Return OK */
	return 1;
}

//
//  Fill the whole screen with the given color
//
void ssd1306_Fill()
{
	/* Set memory */
	uint32_t i;

	for(i = 0; i < sizeof(SSD1306_Buffer); i++)
	{
		SSD1306_Buffer[i] = (SSD1306.Color == Black) ? 0x00 : 0xFF;
	}
}

//
//  Write the screenbuffer with changed to the screen
//
void ssd1306_UpdateScreen(void)
{
	uint8_t i;

	for (i = 0; i < 8; i++)
	{
		ssd1306_WriteCommand(0xB0 + i);
		ssd1306_WriteCommand(SETLOWCOLUMN);
		ssd1306_WriteCommand(SETHIGHCOLUMN);
		ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH * i], width());
	}
}

//
//	Draw one pixel in the screenbuffer
//	X => X Coordinate
//	Y => Y Coordinate
//	color => Pixel color
//
void ssd1306_DrawPixel(uint8_t x, uint8_t y)
{
	SSD1306_COLOR color = SSD1306.Color;

	if (x >= ssd1306_GetWidth() || y >= ssd1306_GetHeight())
	{
		// Don't write outside the buffer
		return;
	}

	// Check if pixel should be inverted
	if (SSD1306.Inverted)
	{
		color = (SSD1306_COLOR) !color;
	}

	// Draw in the right color
	if (color == White)
	{
		SSD1306_Buffer[x + (y / 8) * width()] |= 1 << (y % 8);
	}
	else
	{
		SSD1306_Buffer[x + (y / 8) * width()] &= ~(1 << (y % 8));
	}
}

void ssd1306_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep)
	{
		SWAP_INT16_T(x0, y0);
		SWAP_INT16_T(x1, y1);
	}

	if (x0 > x1)
	{
		SWAP_INT16_T(x0, x1);
		SWAP_INT16_T(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	for (; x0<=x1; x0++)
	{
		if (steep)
		{
			ssd1306_DrawPixel(y0, x0);
		}
		else
		{
			ssd1306_DrawPixel(x0, y0);
		}
		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

void ssd1306_DrawHorizontalLine(int16_t x, int16_t y, int16_t length)
{
  if (y < 0 || y >= height()) { return; }

  if (x < 0)
  {
    length += x;
    x = 0;
  }

  if ( (x + length) > width())
  {
    length = (width() - x);
  }

  if (length <= 0) { return; }

  uint8_t * bufferPtr = SSD1306_Buffer;
  bufferPtr += (y >> 3) * width();
  bufferPtr += x;

  uint8_t drawBit = 1 << (y & 7);

  switch (SSD1306.Color)
  {
    case White:
    	while (length--)
    	{
    		*bufferPtr++ |= drawBit;
    	};
    	break;
    case Black:
    	drawBit = ~drawBit;
    	while (length--)
    	{
    		*bufferPtr++ &= drawBit;
    	};
    	break;
    case Inverse:
    	while (length--)
    	{
    		*bufferPtr++ ^= drawBit;
    	}; break;
  }
}

void ssd1306_DrawVerticalLine(int16_t x, int16_t y, int16_t length)
{
  if (x < 0 || x >= width()) return;

  if (y < 0)
  {
    length += y;
    y = 0;
  }

  if ( (y + length) > height())
  {
    length = (height() - y);
  }

  if (length <= 0) return;


  uint8_t yOffset = y & 7;
  uint8_t drawBit;
  uint8_t *bufferPtr = SSD1306_Buffer;

  bufferPtr += (y >> 3) * width();
  bufferPtr += x;

  if (yOffset)
  {
    yOffset = 8 - yOffset;
    drawBit = ~(0xFF >> (yOffset));

    if (length < yOffset)
    {
      drawBit &= (0xFF >> (yOffset - length));
    }

    switch (SSD1306.Color)
    {
      case White:   *bufferPtr |=  drawBit; break;
      case Black:   *bufferPtr &= ~drawBit; break;
      case Inverse: *bufferPtr ^=  drawBit; break;
    }

    if (length < yOffset) return;

    length -= yOffset;
    bufferPtr += width();
  }

  if (length >= 8)
  {
    switch (SSD1306.Color)
    {
      case White:
      case Black:
        drawBit = (SSD1306.Color == White) ? 0xFF : 0x00;
        do {
          *bufferPtr = drawBit;
          bufferPtr += width();
          length -= 8;
        } while (length >= 8);
        break;
      case Inverse:
        do {
          *bufferPtr = ~(*bufferPtr);
          bufferPtr += width();
          length -= 8;
        } while (length >= 8);
        break;
    }
  }

  if (length > 0)
  {
    drawBit = (1 << (length & 7)) - 1;
    switch (SSD1306.Color)
    {
      case White:   *bufferPtr |=  drawBit; break;
      case Black:   *bufferPtr &= ~drawBit; break;
      case Inverse: *bufferPtr ^=  drawBit; break;
    }
  }
}

void ssd1306_DrawRect(int16_t x, int16_t y, int16_t width, int16_t height)
{
	ssd1306_DrawHorizontalLine(x, y, width);
	ssd1306_DrawVerticalLine(x, y, height);
	ssd1306_DrawVerticalLine(x + width - 1, y, height);
	ssd1306_DrawHorizontalLine(x, y + height - 1, width);
}

void ssd1306_FillRect(int16_t xMove, int16_t yMove, int16_t width, int16_t height)
{
  for (int16_t x = xMove; x < xMove + width; x++)
  {
    ssd1306_DrawVerticalLine(x, yMove, height);
  }
}

void ssd1306_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3)
{
	/* Draw lines */
	ssd1306_DrawLine(x1, y1, x2, y2);
	ssd1306_DrawLine(x2, y2, x3, y3);
	ssd1306_DrawLine(x3, y3, x1, y1);
}

void ssd1306_DrawFillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3)
{
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
	yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
	curpixel = 0;

	deltax = abs(x2 - x1);
	deltay = abs(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1)
	{
		xinc1 = 1;
		xinc2 = 1;
	}
	else
	{
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1)
	{
		yinc1 = 1;
		yinc2 = 1;
	}
	else
	{
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay)
	{
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	}
	else
	{
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++)
	{
		ssd1306_DrawLine(x, y, x3, y3);

		num += numadd;
		if (num >= den)
		{
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
}

void ssd1306_DrawCircle(int16_t x0, int16_t y0, int16_t radius)
{
	int16_t x = 0, y = radius;
	int16_t dp = 1 - radius;
	do
	{
		if (dp < 0)
			dp = dp + 2 * (++x) + 3;
		else
			dp = dp + 2 * (++x) - 2 * (--y) + 5;

		ssd1306_DrawPixel(x0 + x, y0 + y);     //For the 8 octants
		ssd1306_DrawPixel(x0 - x, y0 + y);
		ssd1306_DrawPixel(x0 + x, y0 - y);
		ssd1306_DrawPixel(x0 - x, y0 - y);
		ssd1306_DrawPixel(x0 + y, y0 + x);
		ssd1306_DrawPixel(x0 - y, y0 + x);
		ssd1306_DrawPixel(x0 + y, y0 - x);
		ssd1306_DrawPixel(x0 - y, y0 - x);

	} while (x < y);

	ssd1306_DrawPixel(x0 + radius, y0);
	ssd1306_DrawPixel(x0, y0 + radius);
	ssd1306_DrawPixel(x0 - radius, y0);
	ssd1306_DrawPixel(x0, y0 - radius);
}

void ssd1306_FillCircle(int16_t x0, int16_t y0, int16_t radius)
{
  int16_t x = 0, y = radius;
  int16_t dp = 1 - radius;
  do
  {
	  if (dp < 0)
	  {
		  dp = dp + 2 * (++x) + 3;
	  }
	  else
	  {
		  dp = dp + 2 * (++x) - 2 * (--y) + 5;
	  }

    ssd1306_DrawHorizontalLine(x0 - x, y0 - y, 2*x);
    ssd1306_DrawHorizontalLine(x0 - x, y0 + y, 2*x);
    ssd1306_DrawHorizontalLine(x0 - y, y0 - x, 2*y);
    ssd1306_DrawHorizontalLine(x0 - y, y0 + x, 2*y);


  } while (x < y);
  ssd1306_DrawHorizontalLine(x0 - radius, y0, 2 * radius);
}

void ssd1306_DrawCircleQuads(int16_t x0, int16_t y0, int16_t radius, uint8_t quads)
{
  int16_t x = 0, y = radius;
  int16_t dp = 1 - radius;
  while (x < y)
  {
    if (dp < 0)
      dp = dp + 2 * (++x) + 3;
    else
      dp = dp + 2 * (++x) - 2 * (--y) + 5;
    if (quads & 0x1)
    {
    	ssd1306_DrawPixel(x0 + x, y0 - y);
    	ssd1306_DrawPixel(x0 + y, y0 - x);
    }
    if (quads & 0x2)
    {
    	ssd1306_DrawPixel(x0 - y, y0 - x);
    	ssd1306_DrawPixel(x0 - x, y0 - y);
    }
    if (quads & 0x4)
    {
    	ssd1306_DrawPixel(x0 - y, y0 + x);
    	ssd1306_DrawPixel(x0 - x, y0 + y);
    }
    if (quads & 0x8)
    {
    	ssd1306_DrawPixel(x0 + x, y0 + y);
    	ssd1306_DrawPixel(x0 + y, y0 + x);
    }
  }
  if (quads & 0x1 && quads & 0x8)
  {
	  ssd1306_DrawPixel(x0 + radius, y0);
  }
  if (quads & 0x4 && quads & 0x8)
  {
	  ssd1306_DrawPixel(x0, y0 + radius);
  }
  if (quads & 0x2 && quads & 0x4)
  {
	  ssd1306_DrawPixel(x0 - radius, y0);
  }
  if (quads & 0x1 && quads & 0x2)
  {
	  ssd1306_DrawPixel(x0, y0 - radius);
  }
}

void ssd1306_DrawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress)
{
	uint16_t radius = height / 2;
	uint16_t xRadius = x + radius;
	uint16_t yRadius = y + radius;
	uint16_t doubleRadius = 2 * radius;
	uint16_t innerRadius = radius - 2;

	ssd1306_SetColor(White);
	ssd1306_DrawCircleQuads(xRadius, yRadius, radius, 0b00000110);
	ssd1306_DrawHorizontalLine(xRadius, y, width - doubleRadius + 1);
	ssd1306_DrawHorizontalLine(xRadius, y + height, width - doubleRadius + 1);
	ssd1306_DrawCircleQuads(x + width - radius, yRadius, radius, 0b00001001);

	uint16_t maxProgressWidth = (width - doubleRadius + 1) * progress / 100;

	ssd1306_FillCircle(xRadius, yRadius, innerRadius);
	ssd1306_FillRect(xRadius + 1, y + 2, maxProgressWidth, height - 3);
	ssd1306_FillCircle(xRadius + maxProgressWidth, yRadius, innerRadius);
}

// Draw monochrome bitmap
// input:
//   X, Y - top left corner coordinates of bitmap
//   W, H - width and height of bitmap in pixels
//   pBMP - pointer to array containing bitmap
// note: each '1' bit in the bitmap will be drawn as a pixel
//       each '0' bit in the will not be drawn (transparent bitmap)
// bitmap: one byte per 8 vertical pixels, LSB top, truncate bottom bits
void ssd1306_DrawBitmap(uint8_t X, uint8_t Y, uint8_t W, uint8_t H, const uint8_t* pBMP)
{
	uint8_t pX;
	uint8_t pY;
	uint8_t tmpCh;
	uint8_t bL;

	pY = Y;
	while (pY < Y + H)
	{
		pX = X;
		while (pX < X + W)
		{
			bL = 0;
			tmpCh = *pBMP++;
			if (tmpCh)
			{
				while (bL < 8)
				{
					if (tmpCh & 0x01) ssd1306_DrawPixel(pX,pY + bL);
					tmpCh >>= 1;
					if (tmpCh)
					{
						bL++;
					}
					else
					{
						pX++;
						break;
					}
				}
			}
			else
			{
				pX++;
			}
		}
		pY += 8;
	}
}

char ssd1306_WriteChar(char ch, FontDef Font)
{
	uint32_t i, b, j;

	// Check remaining space on current line
	if (width() <= (SSD1306.CurrentX + Font.FontWidth) ||
		height() <= (SSD1306.CurrentY + Font.FontHeight))
	{
		// Not enough space on current line
		return 0;
	}

	// Use the font to write
	for (i = 0; i < Font.FontHeight; i++)
	{
		b = Font.data[(ch - 32) * Font.FontHeight + i];
		for (j = 0; j < Font.FontWidth; j++)
		{
			if ((b << j) & 0x8000)
			{
				ssd1306_DrawPixel(SSD1306.CurrentX + j, SSD1306.CurrentY + i);
			}
			else
			{
				SSD1306.Color = !SSD1306.Color;
				ssd1306_DrawPixel(SSD1306.CurrentX + j, SSD1306.CurrentY + i);
				SSD1306.Color = !SSD1306.Color;
			}
		}
	}

	// The current space is now taken
	SSD1306.CurrentX += Font.FontWidth;

	// Return written char for validation
	return ch;
}

//
//  Write full string to screenbuffer
//
char ssd1306_WriteString(char* str, FontDef Font)
{
	// Write until null-byte
	while (*str)
	{
		if (ssd1306_WriteChar(*str, Font) != *str)
		{
			// Char could not be written
			return *str;
		}

		// Next char
		str++;
	}

	// Everything ok
	return *str;
}

//
//	Position the cursor
//
void ssd1306_SetCursor(uint8_t x, uint8_t y)
{
	SSD1306.CurrentX = x;
	SSD1306.CurrentY = y;
}

void ssd1306_DisplayOn(void)
{
	ssd1306_WriteCommand(DISPLAYON);
}

void ssd1306_DisplayOff(void)
{
	ssd1306_WriteCommand(DISPLAYOFF);
}

void ssd1306_InvertDisplay(void)
{
	ssd1306_WriteCommand(INVERTDISPLAY);
}

void ssd1306_NormalDisplay(void)
{
	ssd1306_WriteCommand(NORMALDISPLAY);
}

void ssd1306_ResetOrientation()
{
	ssd1306_WriteCommand(SEGREMAP);
	ssd1306_WriteCommand(COMSCANINC);           //Reset screen rotation or mirroring
}

void ssd1306_FlipScreenVertically()
{
	ssd1306_WriteCommand(SEGREMAP | 0x01);
	ssd1306_WriteCommand(COMSCANDEC);           //Rotate screen 180 Deg
}

void ssd1306_MirrorScreen()
{
	ssd1306_WriteCommand(SEGREMAP);
	ssd1306_WriteCommand(COMSCANDEC);           //Mirror screen
}

void ssd1306_Clear()
{
	memset(SSD1306_Buffer, 0, SSD1306_BUFFER_SIZE);
}

//
//  Send a byte to the command register
//
static void ssd1306_WriteCommand(uint8_t command)
{
#ifdef USE_DMA
	while(HAL_I2C_GetState(&SSD1306_I2C_PORT) != HAL_I2C_STATE_READY);
	HAL_I2C_Mem_Write_DMA(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, &command, 1);
#else
	HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, &command, 1, 10);
#endif
}

static void ssd1306_WriteData(uint8_t* data, uint16_t size)
{
#ifdef USE_DMA
	while(HAL_I2C_GetState(&SSD1306_I2C_PORT) != HAL_I2C_STATE_READY);
	HAL_I2C_Mem_Write_DMA(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x40, 1, data, size);
#else
	HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x40, 1, data, size, 100);
#endif
}

#ifdef USE_DMA
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if(hi2c->Instance == SSD1306_I2C_PORT.Instance)
	{
		//TODO:
	}
}
#endif
/* CODE END Public functions */
