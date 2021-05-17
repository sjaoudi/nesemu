#include "ppu.h"
#include "video.h"
#include "system.h"
#include "memory.h"

uint16_t 	scanline;
uint16_t 	ppu_cycle;
uint16_t 	frame;

uint8_t 	nametable_byte;
uint8_t 	attribute_byte;

uint8_t 	background_tile_lo;
uint8_t 	background_tile_hi;

uint16_t 	background_shifter_lo;
uint16_t 	background_shifter_hi;

uint16_t 	attribute_shifter_lo;
uint16_t 	attribute_shifter_hi;

uint8_t		*secondary_oam;
uint8_t		sprite_count;

uint8_t 	sprite_shifters_lo[8];
uint8_t 	sprite_shifters_hi[8];

bool		even_frame;
bool 		render_sprite_zero;

void ppu_reset()
{
	ppu_cycle = 0;
	scanline = 0;
	frame = 0;

	ppu_registers.v = 0x0000;
	ppu_registers.t = 0x0000;
	ppu_registers.x = 0x00;
	ppu_registers.w = 0;

	nametable_byte = 0x00;
	attribute_byte = 0x00;

	background_tile_lo = 0x00;
	background_tile_hi = 0x00;

	background_shifter_lo = 0x0000;
	background_shifter_hi = 0x0000;

	attribute_shifter_lo = 0x0000;
	attribute_shifter_hi = 0x0000;

	secondary_oam = malloc(0xFF);
	if (secondary_oam != NULL)
		memset(secondary_oam, 0xFF, 0xFF);

	sprite_count = 0;

	even_frame = true;
}

static void inc_hori_v()
{
	if ((ppu_registers.v & 0x001F) == 31) // if coarse X == 31
	{
		ppu_registers.v &= ~0x001F;          // coarse X = 0
		ppu_registers.v ^= 0x0400;           // switch horizontal nametable
	}
	else
		ppu_registers.v += 1;                // increment coarse X
}

static void inc_vert_v()
{
	if ((ppu_registers.v & 0x7000) != 0x7000)        // if fine Y < 7
		ppu_registers.v += 0x1000;                // increment fine Y
	else
	{
		ppu_registers.v &= ~0x7000;               // fine Y = 0
		uint16_t y = (ppu_registers.v & 0x03E0) >> 5;  // let y = coarse Y
		if (y == 29)
		{
			y = 0;                            // coarse Y = 0
			ppu_registers.v ^= 0x0800;        // switch vertical nametable
		}
		else if (y == 31)
			y = 0;                            // coarse Y = 0, nametable not switched
		else
			y += 1;                           // increment coarse Y

		ppu_registers.v = (ppu_registers.v & ~0x03E0) | (y << 5);     // put coarse Y back into v
	}
}

static void reset_hori_v()
{
	ppu_registers.v &= ~0x001F; // coarse X = 0
	ppu_registers.v |= (ppu_registers.t & 0x001F);

	ppu_registers.v &= ~0x0400; // nametable X = 0
	ppu_registers.v |= (ppu_registers.t & 0x0400);
}

static void reset_vert_v()
{
	ppu_registers.v &= ~0x7000;
	ppu_registers.v |= (ppu_registers.t & 0x7000);

	ppu_registers.v &= ~0x0800;
	ppu_registers.v |= (ppu_registers.t & 0x0800);

	ppu_registers.v &= ~0x03E0;
	ppu_registers.v |= (ppu_registers.t & 0x03E0);
}

static void shift_background_shifters()
{
	if (is_ppu_flag_set(PPUMASK, PPUMASK_FLAG_B))
	{
		background_shifter_lo <<= 1;
		background_shifter_hi <<= 1;

		attribute_shifter_lo <<= 1;
		attribute_shifter_hi <<= 1;
	}
}

static void shift_sprite_shifters()
{
	if (is_ppu_flag_set(PPUMASK, PPUMASK_FLAG_S))
	{
		for (uint8_t i = 0; i < 8; i++)
		{
			struct OAM_Entry entry;
			memcpy(&entry, &secondary_oam[i * 4], 4);

			if ((ppu_cycle - 1 >= entry.x) && (ppu_cycle - 1 <= entry.x + 7))
			{
				sprite_shifters_lo[i] <<= 1;
				sprite_shifters_hi[i] <<= 1;
			}
		}
	}
}

void ppu_clock()
{
	if (scanline == 241 && ppu_cycle == 1)
	{
		set_ppu_flag(PPUSTATUS, PPUSTATUS_FLAG_V, true);

		if (is_ppu_flag_set(PPUCTRL, PPUCTRL_FLAG_V))
			trigger_nmi = true;
	}

	if (scanline == 261 && ppu_cycle == 1)
	{
		set_ppu_flag(PPUSTATUS, PPUSTATUS_FLAG_V, false);
		set_ppu_flag(PPUSTATUS, PPUSTATUS_FLAG_S, false);
		set_ppu_flag(PPUSTATUS, PPUSTATUS_FLAG_O, false);
	}

	if (is_ppu_flag_set(PPUMASK, PPUMASK_FLAG_B) | is_ppu_flag_set(PPUMASK, PPUMASK_FLAG_S))
	{
		if (scanline <= 239 || scanline == 261)
		{
			if (scanline <= 239 && ppu_cycle >= 1 && ppu_cycle <= 256)
			{
				uint8_t p0, p1;
				uint8_t a0, a1;

				uint8_t background_pixel = 0x00;
				uint8_t background_attribute = 0x00;

				if (is_ppu_flag_set(PPUMASK, PPUMASK_FLAG_B))
				{
					p0 = (background_shifter_lo >> (15 - ppu_registers.x)) & 0x1;
					p1 = (background_shifter_hi >> (15 - ppu_registers.x)) & 0x1;

					a0 = (attribute_shifter_lo >> (15 - ppu_registers.x)) & 0x1;
					a1 = (attribute_shifter_hi >> (15 - ppu_registers.x)) & 0x1;

					background_pixel = (p1 << 1) | p0;
					background_attribute = (a1 << 1) | a0;
				}

				uint8_t sprite_pixel = 0x00;
				uint8_t sprite_attribute = 0x00;

				if (is_ppu_flag_set(PPUMASK, PPUMASK_FLAG_S))
				{
					for (uint8_t i = 0; i < sprite_count; i++)
					{
						struct OAM_Entry entry;
						memcpy(&entry, &secondary_oam[i * 4], 4);

						if ((ppu_cycle - 1 >= entry.x) && (ppu_cycle - 1 <= entry.x + 7))
						{
							p0 = (sprite_shifters_lo[i] >> 7) & 0x1;
							p1 = (sprite_shifters_hi[i] >> 7) & 0x1;

							sprite_pixel = (p1 << 1) | p0;
							sprite_attribute = (entry.attribute & 0x03) + 0x04;
						}
					}
				}
				
				uint8_t pixel = 0x00;
				uint8_t attribute = 0x00;
				
				if (background_pixel > 0x00 && sprite_pixel == 0x00)
				{
					pixel = background_pixel;
					attribute = background_attribute;
				}
				else if (background_pixel > 0x00 && sprite_pixel > 0x00)
				{
					if (render_sprite_zero)
						set_ppu_flag(PPUSTATUS, PPUSTATUS_FLAG_S, true);

					pixel = sprite_pixel;
					attribute = sprite_attribute;
				}
				else if (sprite_pixel > 0x00)
				{
					pixel = sprite_pixel;
					attribute = sprite_attribute;
				}

				uint32_t color = palette[ppu_read(0x3F00 + attribute * 4 + pixel)];
				uint32_t offset = scanline * 256 * 3 + (ppu_cycle - 1) * 3;

				screen[offset] = color >> 16;
				screen[offset + 1] = color >> 8;
				screen[offset + 2] = color;
			}

			switch (ppu_cycle)
			{
				case 1 ... 256:
					shift_sprite_shifters();
				case 321 ... 336:
					shift_background_shifters();
					break;
			}

			switch (ppu_cycle)
			{
				case 8:		case 16:	case 24:	case 32:	case 40:	case 48:	case 56:	case 64:
				case 72:	case 80:	case 88:	case 96:	case 104:	case 112:	case 120:	case 128:
				case 136:	case 144:	case 152:	case 160:	case 168:	case 176:	case 184:	case 192:
				case 200:	case 208:	case 216:	case 224:	case 232:	case 240:	case 248:	case 256:
				case 328:	case 336:
					background_shifter_lo |= background_tile_lo;
					background_shifter_hi |= background_tile_hi;

					attribute_shifter_lo |= (attribute_byte & 0x1 ? 0xFF : 0x00);
					attribute_shifter_hi |= (attribute_byte & 0x2 ? 0xFF : 0x00);
					break;
			}

			switch (ppu_cycle)
			{
				case 1:		case 9:		case 17:	case 25:	case 33:	case 41:	case 49:	case 57:
				case 65:	case 73:	case 81:	case 89:	case 97:	case 105:	case 113:	case 121:
				case 129:	case 137:	case 145:	case 153:	case 161:	case 169:	case 177:	case 185:
				case 193:	case 201:	case 209:	case 217:	case 225:	case 233:	case 241:	case 249:
				case 321:	case 329:
					nametable_byte = ppu_read(0x2000 | (ppu_registers.v & 0x0FFF));
					break;
				case 3:		case 11:	case 19:	case 27:	case 35:	case 43:	case 51:	case 59:
				case 67:	case 75:	case 83:	case 91:	case 99:	case 107:	case 115:	case 123:
				case 131:	case 139:	case 147:	case 155:	case 163:	case 171:	case 179:	case 187:
				case 195:	case 203:	case 211:	case 219:	case 227:	case 235:	case 243:	case 251:
				case 323:	case 331:
					attribute_byte = ppu_read(0x23C0 | (ppu_registers.v & 0x0C00) | 
								 ((ppu_registers.v >> 4) & 0x38) | ((ppu_registers.v >> 2) & 0x07));

					uint8_t tile_x = ppu_registers.v & 0x1F;
					uint8_t tile_y = (ppu_registers.v >> 5) & 0x1F;

					if (tile_x % 4 >= 2 && tile_y % 4 <= 1) // top right
						attribute_byte >>= 2;
					else if (tile_x % 4 <= 1 && tile_y % 4 >= 2) // bottom left
						attribute_byte >>= 4;
					else if (tile_x % 4 >= 2 && tile_y % 4 >= 2) // bottom right
						attribute_byte >>= 6;

					break;
				case 5:		case 13:	case 21:	case 29:	case 37:	case 45:	case 53:	case 61:
				case 69:	case 77:	case 85:	case 93:	case 101:	case 109:	case 117:	case 125:
				case 133:	case 141:	case 149:	case 157:	case 165:	case 173:	case 181:	case 189:
				case 197:	case 205:	case 213:	case 221:	case 229:	case 237:	case 245:	case 253:
				case 325:	case 333:
					background_tile_lo = ppu_read((is_ppu_flag_set(PPUCTRL, PPUCTRL_FLAG_B) ? 0x1000 : 0x0000) +
								      ((uint16_t)nametable_byte << 4) +
								      (((ppu_registers.v >> 12) & 0x7)));
					break;
				case 7:		case 15:	case 23:	case 31:	case 39:	case 47:	case 55:	case 63:
				case 71:	case 79:	case 87:	case 95:	case 103:	case 111:	case 119:	case 127:
				case 135:	case 143:	case 151:	case 159:	case 167:	case 175:	case 183:	case 191:
				case 199:	case 207:	case 215:	case 223:	case 231:	case 239:	case 247:	case 255:
				case 327:	case 335:
					background_tile_hi = ppu_read((is_ppu_flag_set(PPUCTRL, PPUCTRL_FLAG_B) ? 0x1000 : 0x0000) +
								      ((uint16_t)nametable_byte << 4) +
								      (((ppu_registers.v >> 12) & 0x7) + 8));
					break;
				case 8:		case 16:	case 24:	case 32:	case 40:	case 48:	case 56:	case 64:
				case 72:	case 80:	case 88:	case 96:	case 104:	case 112:	case 120:	case 128:
				case 136:	case 144:	case 152:	case 160:	case 168:	case 176:	case 184:	case 192:
				case 200:	case 208:	case 216:	case 224:	case 232:	case 240:	case 248:
				case 328:	case 336:
					inc_hori_v();
					break;
				case 256:
					inc_vert_v();
				case 257:
					reset_hori_v();
					break;
				case 337:	case 339:
					ppu_read(0x2000 | (ppu_registers.v & 0x0FFF));
					break;
			}

			if (scanline == 261)
			{
				if (ppu_cycle >= 280 && ppu_cycle <= 304)
					reset_vert_v();
			}

			if (ppu_cycle == 257)
			{
				memset(secondary_oam, 0xFF, 64 * 4);
				sprite_count = 0;
				render_sprite_zero = false;

				for (uint8_t i = 0; i < 64; i++)
				{
					struct OAM_Entry entry;
					memcpy(&entry, &primary_oam[i * 4], 4);

					uint8_t height = is_ppu_flag_set(PPUCTRL, PPUCTRL_FLAG_H) ? 16 : 8;

					uint8_t sprite_shifter_pattern_lo;
					uint8_t sprite_shifter_pattern_hi;

					if ((scanline >= entry.y) && (scanline <= (entry.y + height - 1)))
					{
						if (sprite_count < 8)
						{
							if (i == 0)
								render_sprite_zero = true;

							// copy to secondary oam ram
							memcpy(&secondary_oam[sprite_count * 4], &entry, 4);

							uint16_t sprite_shifter_addr;

							// flip vertically
							if (entry.attribute & 0x80)
							{
								sprite_shifter_addr = (is_ppu_flag_set(PPUCTRL, PPUCTRL_FLAG_S) ? 0x1000 : 0x0000) + 
											 ((uint16_t)entry.tile << 4) + 
											 (7 - scanline - entry.y);
							}
							else 
							{
								sprite_shifter_addr = (is_ppu_flag_set(PPUCTRL, PPUCTRL_FLAG_S) ? 0x1000 : 0x0000) + 
											 ((uint16_t)entry.tile << 4) + 
											 (scanline - entry.y);
							}

							sprite_shifter_pattern_lo = ppu_read(sprite_shifter_addr);
							sprite_shifter_pattern_hi = ppu_read(sprite_shifter_addr + 8);

							// flip horizontally
							if (entry.attribute & 0x40)
							{
								sprite_shifter_pattern_lo = lut_reverse8[sprite_shifter_pattern_lo];
								sprite_shifter_pattern_hi = lut_reverse8[sprite_shifter_pattern_hi];
							}

							sprite_shifters_lo[sprite_count] = sprite_shifter_pattern_lo;
							sprite_shifters_hi[sprite_count] = sprite_shifter_pattern_hi;

							sprite_count++;
						}
					}
				}

				if (sprite_count > 8)
				{
					sprite_count = 8;
					set_ppu_flag(PPUSTATUS, PPUSTATUS_FLAG_O, true);
				}
			}
		}
	}

	if (!even_frame && scanline == 261 && ppu_cycle == 339 && is_ppu_flag_set(PPUMASK, PPUMASK_FLAG_B))
	{
		ppu_cycle = 0;
		scanline = 0;
		frame++;
		even_frame = !even_frame;
		video_display_frame();
	}
	else if (scanline == 261 && ppu_cycle == 340)
	{
		ppu_cycle = 0;
		scanline = 0;
		frame++;
		even_frame = !even_frame;
		video_display_frame();
	}
	else if (ppu_cycle == 340)
	{
		ppu_cycle = 0;
		scanline++;
	}
	else
	{
		ppu_cycle++;
	}
}

