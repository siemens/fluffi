/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Michael Kraus, Pascal Eckmann, Thomas Riedmaier
*/

§§#include "stdafx.h"
§§#include "sslwrap.h"
§§
§§static unsigned char clientPrivateKey[] = { 0x30, 0x82, 0x04, 0xBE, 0x02, 0x01, 0x00, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x04, 0x82, 0x04, 0xA8, 0x30, 0x82, 0x04, 0xA4, 0x02, 0x01, 0x00, 0x02, 0x82, 0x01, 0x01, 0x00, 0xA5, 0x2D, 0xA0, 0x76, 0xD9, 0x82, 0x32, 0x8D, 0xCE, 0x7C, 0xF1, 0x05, 0x96, 0x4C, 0xB3, 0x81, 0x9D, 0xD4, 0x6B, 0x84, 0xB8, 0x74, 0xD2, 0x10, 0xA4, 0x97, 0x53, 0xB1, 0x84, 0x55, 0x0C, 0x4E, 0xEA, 0x1E, 0x28, 0x36, 0xF0, 0x33, 0x1E, 0x7F, 0x0B, 0xBB, 0x6C, 0xB2, 0xFE, 0x6B, 0x7C, 0x36, 0xFA, 0xC2, 0xCC, 0xE3, 0xF5, 0xCC, 0x5F, 0x8C, 0x70, 0xAA, 0x48, 0x16, 0xC2, 0x30, 0xE5, 0x92, 0xA8, 0x7D, 0x18, 0x53, 0x1C, 0x87, 0xA0, 0xE5, 0x80, 0x76, 0x4D, 0x40, 0x60, 0xD4, 0x77, 0x6D, 0x49, 0x89, 0xEA, 0xD8, 0x85, 0xC8, 0xA4, 0x99, 0x5B, 0x8E, 0xAA, 0xFD, 0xBF, 0x76, 0x61, 0x2F, 0x86, 0x75, 0x9A, 0xAE, 0xBE, 0x1D, 0xD5, 0xD8, 0x31, 0xB5, 0x1E, 0x09, 0x0F, 0xFB, 0x2E, 0xAE, 0x9D, 0xD7, 0xB7, 0x53, 0x13, 0x69, 0xEE, 0xC9, 0x60, 0xF7, 0xF0, 0x2D, 0x4D, 0x87, 0x0D, 0x8E, 0xD5, 0x86, 0x57, 0x8E, 0x53, 0x40, 0xE7, 0xE6, 0x6E, 0x31, 0x1C, 0x52, 0x5A, 0x23, 0x22, 0x5D, 0xE4, 0x2E, 0x2D, 0xF3, 0x01, 0xE6, 0x11, 0x09, 0x8D, 0x2D, 0xC2, 0x93, 0xD7, 0xCE, 0xFB, 0xAF, 0x6B, 0xBE, 0x8A, 0xF5, 0x52, 0x98, 0x03, 0x8C, 0x51, 0x99, 0x4A, 0x6A, 0x20, 0xCD, 0x21, 0x39, 0xCC, 0xD6, 0xDB, 0xE9, 0xD6, 0x4B, 0x98, 0x8C, 0x38, 0xFA, 0xC8, 0xC8, 0xA8, 0x55, 0x3A, 0x21, 0xB0, 0xFA, 0xBA, 0x73, 0x3D, 0xEA, 0x0F, 0x9C, 0xA8, 0x70, 0x24, 0x44, 0x54, 0xCA, 0xBB, 0xD2, 0x90, 0xE2, 0xAD, 0x2F, 0xB6, 0x82, 0xC0, 0x33, 0x64, 0xF4, 0xCF, 0xE4, 0x58, 0xD0, 0xBE, 0xE8, 0xE8, 0x57, 0x8A, 0x9E, 0xEE, 0x31, 0xC5, 0xB7, 0xD1, 0xEF, 0x05, 0xD3, 0x0A, 0x1B, 0xB2, 0x1B, 0x03, 0x1F, 0x3B, 0xDE, 0x39, 0xA4, 0x7D, 0x38, 0x38, 0xD9, 0x50, 0xE4, 0x28, 0xA0, 0xE8, 0xB3, 0x02, 0x03, 0x01, 0x00, 0x01, 0x02, 0x82, 0x01, 0x01, 0x00, 0x92, 0x15, 0x75, 0x4A, 0x47, 0x79, 0xDF, 0x8E, 0x0C, 0xE5, 0xF1, 0x1F, 0xD8, 0xDA, 0x83, 0x13, 0x8A, 0x6B, 0xB8, 0x9F, 0x8B, 0xA7, 0x3D, 0xB5, 0x9C, 0x6B, 0x7D, 0x88, 0x8D, 0x19, 0xCE, 0xA5, 0xE8, 0x66, 0xBD, 0x78, 0x41, 0x1D, 0x64, 0xC6, 0x45, 0xB1, 0x00, 0x24, 0x99, 0xE0, 0xA3, 0xDD, 0xD8, 0x0E, 0xFA, 0xB8, 0x4D, 0xC5, 0xEF, 0x67, 0x3C, 0xA9, 0x4C, 0xD2, 0x5B, 0xF2, 0x74, 0xC7, 0x2D, 0x1E, 0x5D, 0xA9, 0xFB, 0x9C, 0x74, 0x0F, 0x25, 0x7A, 0xFB, 0x3D, 0x89, 0xA5, 0xBE, 0xED, 0xB7, 0xD1, 0x33, 0x13, 0x45, 0xD6, 0xBA, 0xC4, 0x2A, 0xF6, 0x55, 0x81, 0xCF, 0x98, 0x39, 0xC1, 0x97, 0x70, 0x61, 0x5A, 0x54, 0x27, 0xFC, 0xDD, 0x94, 0xB0, 0xB3, 0x1C, 0x9E, 0xB6, 0xAE, 0x85, 0x9E, 0x71, 0x8E, 0xDF, 0xF9, 0x56, 0x81, 0xEA, 0x36, 0x49, 0x71, 0x70, 0x0F, 0x95, 0xF6, 0xC5, 0x45, 0x8A, 0xB4, 0x0D, 0xB5, 0x3B, 0x72, 0xE8, 0x69, 0x15, 0x90, 0xEB, 0x73, 0x73, 0xC8, 0x50, 0x99, 0x68, 0x8D, 0xBE, 0x47, 0xCE, 0x6A, 0x3F, 0x05, 0x3D, 0xC5, 0x78, 0xEB, 0x71, 0x3A, 0xD3, 0xE2, 0x0B, 0x89, 0xC9, 0xAE, 0x59, 0xE4, 0xCE, 0x97, 0xE1, 0x10, 0x2A, 0x2A, 0xEF, 0x1F, 0x4D, 0xF9, 0xEE, 0xF9, 0x71, 0x1C, 0x88, 0x3C, 0x21, 0x04, 0x8F, 0x5B, 0xF7, 0x80, 0xDA, 0x9A, 0x61, 0x7A, 0x57, 0x25, 0x0E, 0x80, 0xFE, 0x7E, 0x2C, 0xDF, 0xEC, 0x25, 0x58, 0xD5, 0x5D, 0xFB, 0x96, 0x5E, 0xA5, 0xA9, 0xC8, 0x7F, 0x3D, 0x74, 0xDC, 0x94, 0xFB, 0xA2, 0x62, 0x6D, 0xF0, 0x5D, 0x71, 0x15, 0x70, 0xED, 0x07, 0xED, 0x0F, 0x39, 0x9A, 0x16, 0x62, 0x11, 0x5F, 0xF4, 0x86, 0x7F, 0x84, 0xE3, 0x1C, 0xB7, 0x86, 0xB7, 0x08, 0xA9, 0xD7, 0x67, 0x12, 0x43, 0x65, 0x21, 0xED, 0x91, 0xB1, 0x02, 0x81, 0x81, 0x00, 0xD0, 0xC4, 0x33, 0xBD, 0x9E, 0xC5, 0xC2, 0xC2, 0x31, 0x71, 0x8C, 0xC5, 0x92, 0x33, 0x6D, 0x63, 0xC7, 0xE6, 0xC0, 0xFA, 0xEA, 0x97, 0x0B, 0x95, 0x5C, 0x06, 0x0E, 0x92, 0x11, 0x2A, 0x04, 0xEF, 0x88, 0xD0, 0x08, 0xE3, 0x69, 0xB0, 0x7E, 0xBB, 0x27, 0x04, 0x86, 0x79, 0x55, 0x57, 0x53, 0xE9, 0x59, 0x2F, 0x9C, 0x3B, 0x84, 0x28, 0x5E, 0x2C, 0x36, 0xA6, 0xDE, 0xE9, 0x93, 0x8F, 0x9F, 0x83, 0x6C, 0x78, 0x97, 0x49, 0x8B, 0x95, 0x47, 0xAF, 0x4E, 0x1D, 0x10, 0xE4, 0xA5, 0x75, 0x5B, 0x6B, 0x23, 0xC4, 0x6C, 0xD9, 0x66, 0x98, 0x6E, 0x7D, 0xFE, 0x6C, 0xAC, 0x5D, 0xF3, 0x16, 0x43, 0xBD, 0xF9, 0xEB, 0xB1, 0x28, 0x9B, 0x16, 0x64, 0xA5, 0xB6, 0xF5, 0xEB, 0xED, 0x33, 0xBE, 0x0F, 0x9F, 0xB9, 0xDC, 0xA3, 0x7F, 0xFB, 0x48, 0x2E, 0xB5, 0x6C, 0xCE, 0x43, 0x75, 0x50, 0x06, 0xDF, 0x39, 0x02, 0x81, 0x81, 0x00, 0xCA, 0x8C, 0xC9, 0x85, 0x84, 0x66, 0x43, 0x13, 0x17, 0x69, 0xDA, 0x7A, 0xF0, 0xA7, 0x49, 0x4B, 0x6C, 0x9E, 0x87, 0x29, 0x47, 0x32, 0x52, 0x29, 0x8B, 0xEB, 0xA5, 0x30, 0xA7, 0xE6, 0x0D, 0x71, 0x40, 0x3F, 0x47, 0x36, 0x11, 0xB7, 0x96, 0x60, 0x3E, 0x8D, 0x02, 0x65, 0xA3, 0xC9, 0x9A, 0xC8, 0x19, 0xCF, 0xE9, 0x88, 0x16, 0xE7, 0x22, 0x92, 0x6D, 0x85, 0x5D, 0xA0, 0x65, 0x12, 0x95, 0x2C, 0x08, 0x9F, 0x85, 0x34, 0x0C, 0x9D, 0x3E, 0xFC, 0xFB, 0xBB, 0x09, 0x48, 0x14, 0x1C, 0x91, 0x80, 0xEC, 0x55, 0x84, 0x30, 0xF1, 0x1C, 0x15, 0x17, 0xBF, 0xCF, 0xB0, 0x4F, 0x1C, 0x7F, 0xD9, 0xF0, 0xCC, 0x64, 0xFE, 0x18, 0x72, 0x29, 0x19, 0x61, 0x80, 0x4C, 0x1D, 0x83, 0xD8, 0xF5, 0xF7, 0xF9, 0xA4, 0x7E, 0x9A, 0x59, 0x6F, 0x88, 0xFE, 0xFC, 0xA0, 0x00, 0x2D, 0x24, 0xC3, 0x26, 0x9B, 0x4B, 0x02, 0x81, 0x81, 0x00, 0x91, 0x92, 0xAE, 0xCD, 0xA3, 0xA0, 0x1C, 0xFD, 0x92, 0xC2, 0xAE, 0x39, 0xBA, 0x87, 0xD7, 0xD8, 0x83, 0x35, 0x3A, 0x0D, 0xFD, 0x75, 0x13, 0xE3, 0xB3, 0x86, 0x99, 0xB4, 0x6A, 0xF9, 0x94, 0xF5, 0x7E, 0xBE, 0x29, 0xE4, 0x81, 0xF7, 0x4B, 0x77, 0xAF, 0x6E, 0x6D, 0x62, 0xC0, 0x0A, 0xA3, 0xFD, 0x8C, 0x6E, 0x31, 0x90, 0x22, 0xC8, 0x8F, 0x5A, 0x5F, 0x45, 0x9F, 0x1C, 0xDB, 0x03, 0x84, 0x64, 0x7D, 0x47, 0xDD, 0x9E, 0xB0, 0x11, 0x89, 0x2A, 0x09, 0xDB, 0x24, 0xC3, 0x7B, 0x48, 0xC9, 0x55, 0x7D, 0x5C, 0x63, 0xD6, 0x78, 0xBF, 0x04, 0x15, 0x21, 0x4B, 0xA0, 0x29, 0xE6, 0xEE, 0xF8, 0x8F, 0xC9, 0x6C, 0xFB, 0x5C, 0xE8, 0x40, 0x06, 0x30, 0xB3, 0xD2, 0xEA, 0x0F, 0xBE, 0x4B, 0xE2, 0xD3, 0x2B, 0x24, 0x5A, 0x72, 0x4C, 0xC3, 0x79, 0xA6, 0x92, 0xD7, 0x85, 0x9C, 0x91, 0x10, 0x29, 0xA9, 0x02, 0x81, 0x80, 0x19, 0xF9, 0x3B, 0xCE, 0x57, 0x50, 0x15, 0xC1, 0xD8, 0xFA, 0xC5, 0xFD, 0x85, 0xE6, 0xAD, 0xCB, 0x72, 0x3E, 0x51, 0xEE, 0xC5, 0x26, 0x7A, 0xCA, 0xC1, 0x1A, 0x92, 0xB0, 0x4E, 0x97, 0x7D, 0x99, 0xCC, 0x37, 0x51, 0x16, 0x69, 0xE7, 0x34, 0xED, 0x5A, 0x98, 0x12, 0x7A, 0x9E, 0x2A, 0x38, 0x22, 0xF8, 0xCC, 0x64, 0x92, 0x02, 0xFD, 0xD8, 0xBD, 0x16, 0x36, 0x47, 0xEF, 0x4D, 0x86, 0xB8, 0x82, 0x51, 0x08, 0xC3, 0xB8, 0x0C, 0xF6, 0x0D, 0x21, 0xBB, 0xD6, 0x1C, 0x36, 0x95, 0xFD, 0xA3, 0xDA, 0xF8, 0x73, 0x6E, 0xE4, 0x27, 0xC7, 0x06, 0x1D, 0x69, 0x87, 0x64, 0x80, 0x80, 0x93, 0x14, 0x8B, 0x80, 0x6A, 0x62, 0xFB, 0x17, 0x88, 0x95, 0xAC, 0x9F, 0x1B, 0x23, 0x9D, 0xC8, 0xB0, 0xA1, 0xFF, 0xED, 0x4C, 0x1E, 0xD6, 0xE1, 0x2A, 0x9C, 0x28, 0x21, 0x0C, 0x64, 0xBD, 0x96, 0x18, 0xD9, 0x49, 0x02, 0x81, 0x80, 0x04, 0x5C, 0x88, 0x56, 0x9F, 0x3E, 0x59, 0x2A, 0x4E, 0x1D, 0x1A, 0xA2, 0xA5, 0x03, 0x25, 0xF6, 0xE3, 0x19, 0x63, 0xFD, 0x07, 0xBB, 0xF7, 0x7F, 0x5A, 0x77, 0x1C, 0xEC, 0x18, 0xE1, 0x40, 0x38, 0xDA, 0xCE, 0xCA, 0x2A, 0x64, 0x33, 0xC4, 0x38, 0xF6, 0xBF, 0x78, 0x3F, 0x7E, 0xA8, 0xAA, 0xC9, 0x0E, 0x75, 0x1D, 0x81, 0x9F, 0xAD, 0x01, 0x69, 0xCC, 0x14, 0x13, 0x42, 0x2C, 0x95, 0xBD, 0x0E, 0x19, 0xBC, 0x3D, 0xD1, 0x36, 0x98, 0xF6, 0xD1, 0x83, 0x48, 0xD7, 0x58, 0x04, 0xAA, 0x70, 0xFF, 0xD3, 0x9F, 0xC0, 0xC4, 0x5E, 0xCC, 0x01, 0xAA, 0xB6, 0xC2, 0x00, 0x2F, 0x45, 0x7F, 0x38, 0x5C, 0xB0, 0x1D, 0xAF, 0xFF, 0x8E, 0xA5, 0x17, 0x2F, 0x57, 0x7F, 0xB8, 0xD3, 0x8A, 0x59, 0x51, 0xE0, 0x70, 0x92, 0x0E, 0xB8, 0xB2, 0xA2, 0xDC, 0x1C, 0xEB, 0x8C, 0xB4, 0xD8, 0x28, 0x42, 0x5A, 0x2E };
§§static unsigned char clientCert[] = { 0x30, 0x82, 0x03, 0xBB, 0x30, 0x82, 0x02, 0xA3, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x09, 0x00, 0xB3, 0x26, 0x13, 0x14, 0x9C, 0xA4, 0xDD, 0xA9, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B, 0x05, 0x00, 0x30, 0x74, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x44, 0x45, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0C, 0x07, 0x42, 0x61, 0x76, 0x61, 0x72, 0x69, 0x61, 0x31, 0x0F, 0x30, 0x0D, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0C, 0x06, 0x4D, 0x75, 0x6E, 0x69, 0x63, 0x68, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x07, 0x53, 0x69, 0x65, 0x6D, 0x65, 0x6E, 0x73, 0x31, 0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x0C, 0x03, 0x43, 0x53, 0x41, 0x31, 0x22, 0x30, 0x20, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x19, 0x53, 0x65, 0x63, 0x72, 0x65, 0x74, 0x43, 0x6F, 0x6D, 0x6D, 0x75, 0x6E, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x43, 0x6C, 0x69, 0x65, 0x6E, 0x74, 0x30, 0x1E, 0x17, 0x0D, 0x31, 0x36, 0x30, 0x38, 0x32, 0x32, 0x31, 0x32, 0x31, 0x39, 0x30, 0x30, 0x5A, 0x17, 0x0D, 0x33, 0x36, 0x30, 0x35, 0x30, 0x39, 0x31, 0x32, 0x31, 0x39, 0x30, 0x30, 0x5A, 0x30, 0x74, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x44, 0x45, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0C, 0x07, 0x42, 0x61, 0x76, 0x61, 0x72, 0x69, 0x61, 0x31, 0x0F, 0x30, 0x0D, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0C, 0x06, 0x4D, 0x75, 0x6E, 0x69, 0x63, 0x68, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x07, 0x53, 0x69, 0x65, 0x6D, 0x65, 0x6E, 0x73, 0x31, 0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x0C, 0x03, 0x43, 0x53, 0x41, 0x31, 0x22, 0x30, 0x20, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x19, 0x53, 0x65, 0x63, 0x72, 0x65, 0x74, 0x43, 0x6F, 0x6D, 0x6D, 0x75, 0x6E, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x43, 0x6C, 0x69, 0x65, 0x6E, 0x74, 0x30, 0x82, 0x01, 0x22, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0F, 0x00, 0x30, 0x82, 0x01, 0x0A, 0x02, 0x82, 0x01, 0x01, 0x00, 0xA5, 0x2D, 0xA0, 0x76, 0xD9, 0x82, 0x32, 0x8D, 0xCE, 0x7C, 0xF1, 0x05, 0x96, 0x4C, 0xB3, 0x81, 0x9D, 0xD4, 0x6B, 0x84, 0xB8, 0x74, 0xD2, 0x10, 0xA4, 0x97, 0x53, 0xB1, 0x84, 0x55, 0x0C, 0x4E, 0xEA, 0x1E, 0x28, 0x36, 0xF0, 0x33, 0x1E, 0x7F, 0x0B, 0xBB, 0x6C, 0xB2, 0xFE, 0x6B, 0x7C, 0x36, 0xFA, 0xC2, 0xCC, 0xE3, 0xF5, 0xCC, 0x5F, 0x8C, 0x70, 0xAA, 0x48, 0x16, 0xC2, 0x30, 0xE5, 0x92, 0xA8, 0x7D, 0x18, 0x53, 0x1C, 0x87, 0xA0, 0xE5, 0x80, 0x76, 0x4D, 0x40, 0x60, 0xD4, 0x77, 0x6D, 0x49, 0x89, 0xEA, 0xD8, 0x85, 0xC8, 0xA4, 0x99, 0x5B, 0x8E, 0xAA, 0xFD, 0xBF, 0x76, 0x61, 0x2F, 0x86, 0x75, 0x9A, 0xAE, 0xBE, 0x1D, 0xD5, 0xD8, 0x31, 0xB5, 0x1E, 0x09, 0x0F, 0xFB, 0x2E, 0xAE, 0x9D, 0xD7, 0xB7, 0x53, 0x13, 0x69, 0xEE, 0xC9, 0x60, 0xF7, 0xF0, 0x2D, 0x4D, 0x87, 0x0D, 0x8E, 0xD5, 0x86, 0x57, 0x8E, 0x53, 0x40, 0xE7, 0xE6, 0x6E, 0x31, 0x1C, 0x52, 0x5A, 0x23, 0x22, 0x5D, 0xE4, 0x2E, 0x2D, 0xF3, 0x01, 0xE6, 0x11, 0x09, 0x8D, 0x2D, 0xC2, 0x93, 0xD7, 0xCE, 0xFB, 0xAF, 0x6B, 0xBE, 0x8A, 0xF5, 0x52, 0x98, 0x03, 0x8C, 0x51, 0x99, 0x4A, 0x6A, 0x20, 0xCD, 0x21, 0x39, 0xCC, 0xD6, 0xDB, 0xE9, 0xD6, 0x4B, 0x98, 0x8C, 0x38, 0xFA, 0xC8, 0xC8, 0xA8, 0x55, 0x3A, 0x21, 0xB0, 0xFA, 0xBA, 0x73, 0x3D, 0xEA, 0x0F, 0x9C, 0xA8, 0x70, 0x24, 0x44, 0x54, 0xCA, 0xBB, 0xD2, 0x90, 0xE2, 0xAD, 0x2F, 0xB6, 0x82, 0xC0, 0x33, 0x64, 0xF4, 0xCF, 0xE4, 0x58, 0xD0, 0xBE, 0xE8, 0xE8, 0x57, 0x8A, 0x9E, 0xEE, 0x31, 0xC5, 0xB7, 0xD1, 0xEF, 0x05, 0xD3, 0x0A, 0x1B, 0xB2, 0x1B, 0x03, 0x1F, 0x3B, 0xDE, 0x39, 0xA4, 0x7D, 0x38, 0x38, 0xD9, 0x50, 0xE4, 0x28, 0xA0, 0xE8, 0xB3, 0x02, 0x03, 0x01, 0x00, 0x01, 0xA3, 0x50, 0x30, 0x4E, 0x30, 0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0x9E, 0xEE, 0x3F, 0x0D, 0x32, 0x1D, 0x7E, 0x49, 0x83, 0xF9, 0x8A, 0x8F, 0xCF, 0x7F, 0x22, 0x67, 0xF6, 0x0C, 0x0A, 0xD3, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x9E, 0xEE, 0x3F, 0x0D, 0x32, 0x1D, 0x7E, 0x49, 0x83, 0xF9, 0x8A, 0x8F, 0xCF, 0x7F, 0x22, 0x67, 0xF6, 0x0C, 0x0A, 0xD3, 0x30, 0x0C, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xFF, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x23, 0xCF, 0x13, 0xA7, 0x6C, 0x44, 0x36, 0x11, 0xAD, 0xCE, 0xAA, 0xA7, 0x14, 0x4A, 0x80, 0xF3, 0xE6, 0x38, 0x2F, 0xB5, 0xE7, 0x43, 0xA2, 0x5D, 0x81, 0x4B, 0xCD, 0x2B, 0xDE, 0xFC, 0xD4, 0x34, 0x20, 0x51, 0x3B, 0x5B, 0xF2, 0x6B, 0xF6, 0xC0, 0x2E, 0x56, 0xD6, 0xB6, 0x65, 0x4B, 0xD5, 0x83, 0x9F, 0x35, 0x29, 0xEE, 0x37, 0xB6, 0x71, 0x92, 0xB5, 0xE1, 0x98, 0x12, 0xB9, 0x0A, 0x8A, 0xEA, 0x34, 0x7E, 0xDA, 0xD0, 0x52, 0xB2, 0xF6, 0xC4, 0x06, 0xF0, 0xEC, 0x9C, 0x37, 0xE7, 0x5F, 0xC5, 0x5F, 0x70, 0x72, 0x95, 0x44, 0x2F, 0xAC, 0x0F, 0x83, 0xEF, 0x6C, 0xD9, 0x09, 0x38, 0x20, 0x4C, 0x54, 0x5D, 0xBA, 0x48, 0x97, 0x2D, 0xDC, 0x5E, 0xE7, 0x57, 0xB8, 0x9A, 0xD5, 0x0A, 0xB3, 0x3D, 0x77, 0x59, 0xFE, 0xED, 0x39, 0xF2, 0x97, 0x6D, 0x2A, 0x55, 0xDC, 0x3B, 0x0F, 0x44, 0x21, 0x78, 0xFC, 0x94, 0x2E, 0x43, 0xEE, 0xEC, 0xE5, 0x92, 0xA9, 0x15, 0xE5, 0x8E, 0x9B, 0x9F, 0x69, 0xD1, 0x8F, 0xAE, 0x41, 0x62, 0x79, 0x9A, 0x46, 0x57, 0xB6, 0xF8, 0xF1, 0xCC, 0xC0, 0xEF, 0xD2, 0xBD, 0x87, 0x85, 0x74, 0x3C, 0x68, 0x47, 0xEA, 0x89, 0xFA, 0xD8, 0x8A, 0xA0, 0x3C, 0x07, 0x49, 0x60, 0x0E, 0x5F, 0x86, 0x9A, 0xA0, 0xFC, 0xF1, 0xC9, 0xC4, 0xAE, 0x0A, 0x77, 0xE3, 0x11, 0x61, 0xE8, 0x53, 0x9A, 0x93, 0x10, 0x04, 0x4A, 0x0E, 0xD4, 0x80, 0xF1, 0x91, 0x1D, 0x08, 0xF3, 0x6C, 0xA9, 0xE0, 0xA6, 0x3D, 0x7E, 0x72, 0x58, 0xF3, 0x1B, 0x8B, 0x2C, 0xF3, 0x0A, 0xE5, 0x60, 0xB1, 0x88, 0x66, 0x56, 0x22, 0xE6, 0x7F, 0xDF, 0x84, 0xCF, 0xC7, 0x7F, 0xC3, 0x31, 0x8B, 0x23, 0x15, 0xA8, 0x88, 0xE3, 0x0B, 0x74, 0x00, 0x99, 0x60, 0x68, 0x67, 0xDF, 0x09, 0x6F, 0x25, 0x44, 0x47, 0x94 };
§§
§§SSL* getConnection(const char* ip, const int conPort, BIO* outbio, BIO* inbio, SOCKET server, SSL_CTX* ctx);
§§SOCKET createSocket(char* dstIP, int port, BIO* outbio);
§§SSL_CTX* createSSLContext(BIO* outbio);
§§int freeStructures(SSL* ssl, SOCKET server, SSL_CTX* ctx);
§§SOCKET create_socket(const char ip[], const int port, BIO* out);
§§void initOpenSSL();
§§BIO* setUpBufIO(SSL* ssl);
§§
§§int sendByteBufOnce(char* dstIP, int port, char* msg, int msgSize)
§§{
§§	/* ---------------------------------------------------------- *
§§	* Create the Input/Output BIO's.                             *
§§	* ---------------------------------------------------------- */
§§	BIO* outbio = BIO_new_fp(stdout, BIO_NOCLOSE);
§§	if (outbio == NULL) { std::cout << ">sslwrap.dll< - Error creating BIO!" << std::endl; return -1; }
§§	BIO* inbio = BIO_new_fp(stdin, BIO_NOCLOSE);
§§	if (inbio == NULL) { std::cout << ">sslwrap.dll< - Error creating BIO!" << std::endl; return -1; }
§§
§§	/* ---------------------------------------------------------- *
§§	* initialize SSL library and register algorithms             *
§§	* ---------------------------------------------------------- */
§§	if (SSL_library_init() < 0) {
§§		BIO_printf(outbio, ">sslwrap.dll< - Could not initialize the OpenSSL library!\n");
§§		return -1;
§§	}
§§
§§	SSL_CTX* ctx = createSSLContext(outbio);
§§	if (ctx == NULL) { std::cout << ">sslwrap.dll< - Error creating SSL context!" << std::endl; return -1; }
§§
§§	SOCKET server = createSocket(dstIP, port, outbio);
§§	if (server == NULL) { std::cout << ">sslwrap.dll< - Error creating SSL socket!" << std::endl; return -1; }
§§
§§	SSL* ssl = getConnection(dstIP, port, outbio, inbio, server, ctx);
§§	if (ssl == NULL) { std::cout << ">sslwrap.dll< - SSL connection Error!" << std::endl; return -1; }
§§
§§	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
§§
§§	BIO* buf_io = setUpBufIO(ssl);
§§	if (buf_io == NULL) { std::cout << ">sslwrap.dll< - Error setting up BIO!" << std::endl; return -1; }
§§
§§	// Request
§§	int n = BIO_puts(buf_io, msg);
§§	if (n < 0) { std::cout << ">sslwrap.dll< - Error writing bytes OR no data available, try again!" << std::endl; return -1; }
§§
§§	int success = BIO_flush(buf_io);
§§	if (success < 1) { std::cout << ">sslwrap.dll< - Error flushing Message!" << std::endl; return -1; }
§§
§§	int free = freeStructures(ssl, server, ctx);
§§	if (free < 0) { std::cout << ">sslwrap.dll< - Error freeing structures!" << std::endl; return -1; }
§§
§§	return 0;
§§}
§§
§§/* ---------------------------------------------------------- *
§§* Saves response in bytebuffer
§§* Returns 0 if succesful, -1 if openssl lib error, -2 if resonse too large for buffer...
§§* ---------------------------------------------------------- */
§§int sendByteBufWithResponse(char* dstIP, int port, char* msg, int msgSize, char* response, int responseSize)
§§{
§§	int retCode = 0;
§§	/* ---------------------------------------------------------- *
§§	* Create the Input/Output BIO's.                             *
§§	* ---------------------------------------------------------- */
§§	BIO* outbio = BIO_new_fp(stdout, BIO_NOCLOSE);
§§	if (outbio == NULL) { std::cout << ">sslwrap.dll< - Error creating BIO" << std::endl; return -1; }
§§	BIO* inbio = BIO_new_fp(stdin, BIO_NOCLOSE);
§§	if (inbio == NULL) { std::cout << ">sslwrap.dll< - Error creating BIO" << std::endl; return -1; }
§§
§§	/* ---------------------------------------------------------- *
§§	* initialize SSL library and register algorithms             *
§§	* ---------------------------------------------------------- */
§§	if (SSL_library_init() < 0) {
§§		BIO_printf(outbio, ">sslwrap.dll< - Could not initialize the OpenSSL library !\n");
§§		return -1;
§§	}
§§
§§	SSL_CTX* ctx = createSSLContext(outbio);
§§	if (ctx == NULL) { std::cout << ">sslwrap.dll< - Error creating SSL context" << std::endl; return -1; }
§§
§§	SOCKET server = createSocket(dstIP, port, outbio);
§§	if (server == NULL) { std::cout << ">sslwrap.dll< - Error creating SSL socket!" << std::endl; return -1; }
§§
§§	SSL* ssl = getConnection(dstIP, port, outbio, inbio, server, ctx);
§§	if (ssl == NULL) { std::cout << ">sslwrap.dll< - SSL connection Error!" << std::endl; return -1; }
§§
§§	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
§§
§§	BIO* buf_io = setUpBufIO(ssl);
§§	if (buf_io == NULL) { std::cout << ">sslwrap.dll< - Error setting up BIO!" << std::endl; return -1; }
§§
§§	// Request
§§	int n = BIO_puts(buf_io, msg);
§§	if (n < 0) { std::cout << ">sslwrap.dll< - Error writing bytes OR no data available, try again!" << std::endl; return -1; }
§§
§§	int success = BIO_flush(buf_io);
§§	if (success < 1) { std::cout << ">sslwrap.dll< - Error flushing Message!" << std::endl; return -1; }
§§
§§	int lenRead = 0;
§§	int currentLen = 0;
§§	do
§§	{
§§		if (!(lenRead + 1 >= responseSize))
§§		{
§§			currentLen = BIO_gets(buf_io, response + lenRead, responseSize - lenRead);
§§			if (currentLen < 1)
§§			{
§§				if (BIO_should_retry(buf_io))
§§				{
§§					continue;
§§				}
§§			}
§§			lenRead += currentLen;
§§		}
§§		else
§§		{
§§			// Response does not fit into buffer.
§§			// Does not interrupt the processing currently because in most cases only the headers are important.
§§			// Feeder will interrupt if no cookie etc was found.
§§			std::cout << ">sslwrap.dll< - Warning: Server response larger than the buffer. Response cropped! Increase Buffer Size!" << std::endl;
§§			retCode = -2;
§§			break;
§§		}
§§	} while ((currentLen > 0) && (BIO_pending(buf_io) > 0));
§§
§§	msgSize = lenRead;
§§
§§	int free = freeStructures(ssl, server, ctx);
§§	if (free < 0) { std::cout << ">sslwrap.dll< - Error freeing structures!" << std::endl; return -1; }
§§
§§	return retCode;
§§}
§§
§§/* ---------------------------------------------------------- *
§§* Set up buffere IO
§§* ---------------------------------------------------------- */
§§BIO* setUpBufIO(SSL* ssl)
§§{
	BIO* buf_io;
	BIO* ssl_bio;
§§	buf_io = BIO_new(BIO_f_buffer());  /* create a buffer BIO */
§§	if (buf_io == NULL) { std::cout << ">sslwrap.dll< - Error creating new BIO!" << std::endl; return NULL; }
§§	ssl_bio = BIO_new(BIO_f_ssl());           /* create an ssl BIO */
§§	if (ssl_bio == NULL) { std::cout << ">sslwrap.dll< - Error creating new SSL BIO!" << std::endl; return NULL; }
§§	BIO_set_ssl(ssl_bio, ssl, BIO_CLOSE);       /* assign the ssl BIO to SSL */
§§	BIO_push(buf_io, ssl_bio);          /* add ssl_bio to buf_io */
§§
§§	return buf_io;
§§}
§§
§§/* ---------------------------------------------------------- *
§§* These function calls initialize openssl for correct work.  *
§§* ---------------------------------------------------------- */
§§void initOpenSSL()
§§{
§§	OpenSSL_add_all_algorithms();
§§	ERR_load_BIO_strings();
§§	ERR_load_crypto_strings();
§§	SSL_load_error_strings();
§§}
§§
§§/* ---------------------------------------------------------- *
§§* Make the underlying TCP socket connection                  *
§§* ---------------------------------------------------------- */
§§SOCKET createSocket(char* dstIP, int port, BIO* outbio)
§§{
§§	SOCKET server = create_socket(dstIP, port, outbio);
§§	if (server != INVALID_SOCKET) {
§§		BIO_printf(outbio, ">sslwrap.dll< - Successfully made the TCP connection to: %s:%d.\n", dstIP, port);
§§	}
§§	else {
§§		return NULL;
§§	}
§§	return server;
§§}
§§
§§/* ---------------------------------------------------------- *
§§* Try to create a new SSL context                            *
§§* ---------------------------------------------------------- */
§§SSL_CTX* createSSLContext(BIO* outbio)
§§{
§§	/* ---------------------------------------------------------- *
§§	* Set TLSv1_2 client method      *
§§	* ---------------------------------------------------------- */
§§	const SSL_METHOD* method = TLSv1_2_client_method();
§§
§§	/* ---------------------------------------------------------- *
§§	* Try to create a new SSL context                            *
§§	* ---------------------------------------------------------- */
§§	SSL_CTX* ctx;
§§	if ((ctx = SSL_CTX_new(method)) == NULL) {
§§		BIO_printf(outbio, ">sslwrap.dll< - Unable to create a new SSL context structure.\n");
§§		return NULL;
§§	}
§§
§§	return ctx;
§§}
§§
§§int freeStructures(SSL* ssl, SOCKET server, SSL_CTX* ctx)
§§{
§§	/* ---------------------------------------------------------- *
§§	* Free the structures we don't need anymore                  *
§§	* -----------------------------------------------------------*/
§§	SSL_free(ssl);
§§	if (closesocket(server) != 0) {
§§		std::cout << ">sslwrap.dll< - Error closing Socket" << std::endl;
§§		return -1;
§§	};
§§
§§	SSL_CTX_free(ctx);
§§	if (WSACleanup() != 0) {
§§		std::cout << ">sslwrap.dll< - Error in WSACleanup(Windows)" << std::endl;
§§		return -1;
§§	};
§§
§§	return 0;
§§}
§§
§§//Certificate pinning taken from https://www.owasp.org/index.php/Certificate_and_Public_Key_Pinning
§§int pkp_pin_peer_pubkey(SSL* ssl)
§§{
§§	if (NULL == ssl) return FALSE;
§§
§§	X509* cert = NULL;
§§
§§	/* Scratch */
§§	int len1 = 0, len2 = 0;
§§	unsigned char* receivedCertBytes = NULL;
§§
§§	/* Result is returned to caller */
§§	int ret = 0, result = TRUE;
§§
§§	do
§§	{
§§		/* http://www.openssl.org/docs/ssl/SSL_get_peer_certificate.html */
§§		cert = SSL_get_peer_certificate(ssl);
§§		if (!(cert != NULL)) {
§§			break; /* failed */
§§		}
§§
§§		/* Begin Gyrations to get the subjectPublicKeyInfo       */
§§		/* Thanks to Viktor Dukhovni on the OpenSSL mailing list */
§§
§§		/* http://groups.google.com/group/mailing.openssl.users/browse_thread/thread/d61858dae102c6c7 */
§§		len1 = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(cert), NULL);
§§		if (!(len1 > 0)) {
§§			break; /* failed */
§§		}
§§
§§		/* scratch */
§§		unsigned char* temp = NULL;
§§
§§		/* http://www.openssl.org/docs/crypto/buffer.html */
§§		receivedCertBytes = temp = (unsigned char*)OPENSSL_malloc(len1);
§§		if (!(receivedCertBytes != NULL)) {
§§			break; /* failed */
§§		}
§§
§§		/* http://www.openssl.org/docs/crypto/d2i_X509.html */
§§		len2 = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(cert), &temp);
§§
§§		/* These checks are verifying we got back the same values as when we sized the buffer.      */
§§		/* Its pretty weak since they should always be the same. But it gives us something to test. */
§§		if (!((len1 == len2) && (temp != NULL) && ((temp - receivedCertBytes) == len1))) {
§§			break; /* failed */
§§		}
§§
§§		/* The one good exit point */
§§		result = TRUE;
§§	} while (0);
§§
§§	/* http://www.openssl.org/docs/crypto/buffer.html */
§§	if (NULL != receivedCertBytes) {
§§		OPENSSL_free(receivedCertBytes);
§§	}
§§
§§	/* http://www.openssl.org/docs/crypto/X509_new.html */
§§	if (NULL != cert) {
§§		X509_free(cert);
§§	}
§§
§§	return result;
§§}
§§
§§/* ---------------------------------------------------------- *
§§* create_socket() creates the socket & TCP-connect to server *
§§* ---------------------------------------------------------- */
§§SOCKET create_socket(const char ip[], const int port, BIO* out) {
§§	//----------------------
§§	// Initialize Winsock
§§	WSADATA wsaData;
§§	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
§§	if (iResult != NO_ERROR) {
§§		BIO_printf(out, ">sslwrap.dll< - WSAStartup function failed with error: %d\n", iResult);
§§		return INVALID_SOCKET;
§§	}
§§
§§	/* ---------------------------------------------------------- *
§§	* create the basic TCP socket                                *
§§	* ---------------------------------------------------------- */
§§	SOCKET sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
§§	if (sockfd == INVALID_SOCKET) {
§§		BIO_printf(out, ">sslwrap.dll< - Socket function failed with error: %ld\n", WSAGetLastError());
§§		WSACleanup();
§§		return INVALID_SOCKET;
§§	}
§§
§§	struct sockaddr_in dest_addr;
§§	dest_addr.sin_family = AF_INET;
§§	dest_addr.sin_port = htons(port);
§§	dest_addr.sin_addr.s_addr = inet_addr(ip);
§§
§§	char* tmp_ptr = inet_ntoa(dest_addr.sin_addr);
§§
§§	/* ---------------------------------------------------------- *
§§	* Try to connect to the server                    *
§§	* ---------------------------------------------------------- */
§§	if (connect(sockfd, (struct sockaddr*) &dest_addr, sizeof(struct sockaddr)) == SOCKET_ERROR) {
§§		BIO_printf(out, ">sslwrap.dll< - Error: Cannot connect to ip %s [%s] on port %d.\n", ip, tmp_ptr, port);
§§		if (closesocket(sockfd) == SOCKET_ERROR)
§§			BIO_printf(out, ">sslwrap.dll< - closesocket function failed with error: %ld\n", WSAGetLastError());
§§		WSACleanup();
§§		return INVALID_SOCKET;
§§	}
§§
§§	return sockfd;
§§}
§§
§§/* ---------------------------------------------------------- *
§§* getConnection() returns the connection to the server *
§§* ---------------------------------------------------------- */
§§SSL* getConnection(const char* ip, const int conPort, BIO* outbio, BIO* inbio, SOCKET server, SSL_CTX* ctx) {
§§	const char* dstIP = ip;
§§	const int port = conPort;
§§
§§	/* ---------------------------------------------------------- *
§§	* Set the client's certificate and private key                *
§§	* ---------------------------------------------------------- */
§§	if (SSL_CTX_use_certificate_ASN1(ctx, sizeof(clientCert), clientCert) != 1) {
§§		BIO_printf(outbio, ">sslwrap.dll< - Unable to set the client's certificate.\n");
§§		return NULL;
§§	}
§§	if (SSL_CTX_use_PrivateKey_ASN1(EVP_PKEY_RSA, ctx, clientPrivateKey, sizeof(clientPrivateKey)) != 1) {
§§		BIO_printf(outbio, ">sslwrap.dll< - Unable to set the client's private key.\n");
§§		return NULL;
§§	}
§§
§§	/* ---------------------------------------------------------- *
§§	* Create new SSL connection state object                     *
§§	* ---------------------------------------------------------- */
§§	SSL* ssl = SSL_new(ctx);
§§
§§	/*---------------------------------------------------------- *
§§	* Attach the SSL session to the socket descriptor            *
§§	* ---------------------------------------------------------- */
§§	SSL_set_fd(ssl, (int)server);
§§
§§	/* ---------------------------------------------------------- *
§§	* Try to SSL-connect here, returns 1 for success             *
§§	* ---------------------------------------------------------- */
§§	if (SSL_connect(ssl) != 1) {
§§		BIO_printf(outbio, ">sslwrap.dll< - Error: Could not build a SSL session to: %s:%d.\n", dstIP, port);
§§		return NULL;
§§	}
§§	else {
§§		BIO_printf(outbio, ">sslwrap.dll< - Successfully enabled SSL/TLS session to: %s:%d.\n", dstIP, port);
§§	}
§§
§§	/* ---------------------------------------------------------- *
§§	* Pin the server's certificate
§§	* ---------------------------------------------------------- */
§§	if (!pkp_pin_peer_pubkey(ssl)) {
§§		BIO_printf(outbio, ">sslwrap.dll< - Error: Server certificate verification failed!\n");
§§		return NULL;
§§	}
§§
§§	return ssl;
§§}
§§
§§void UnixTimeToFileTime(time_t t, LPFILETIME pft)
§§{
§§	// Note that LONGLONG is a 64-bit value
§§	LONGLONG ll;
§§
§§	ll = Int32x32To64(t, 10000000) + 116444736000000000;
§§	pft->dwLowDateTime = (DWORD)ll;
§§	pft->dwHighDateTime = ll >> 32;
}