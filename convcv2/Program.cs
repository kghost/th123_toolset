using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Drawing.Imaging;
using System.Drawing;
using System.Runtime.InteropServices;

namespace convcv2
{
    class Program
    {
        // ここからCV2リーダ
        public class CV2Reader
        {
            private CV2Header _header;
            Bitmap _image;
            public CV2Header Header
            {
                get { return _header; }
            }

            public CV2Reader(Stream input)
            {
                if (input.Length < 17)
                    throw new IOException("ファイルが小さすぎます");
                _header = CV2Header.Deserialize(input);
                if (_header.Height * _header.PaddedWidth * _header.Bpp > input.Length - 17)
                    throw new IOException("ファイルが小さすぎます");

                input.Seek(17, SeekOrigin.Begin);
                _image = new Bitmap(_header.Width, _header.Height, _header.PixelFormat);
                int lineSize = _header.Width * _header.Bpp;
                byte[] lineBuf = new byte[_header.PaddedWidth * _header.Bpp];
                for (int y = 0; y < _header.Height; ++y)
                {
                    input.Read(lineBuf, 0, lineBuf.Length);
                    BitmapData data = _image.LockBits(
                         new Rectangle(0, y, _header.Width, 1),
                         ImageLockMode.WriteOnly, _header.PixelFormat);
                    Marshal.Copy(lineBuf, 0, data.Scan0, lineSize);
                    _image.UnlockBits(data);
                }
            }

            public Bitmap Image
            {
                get { return _image; }
            }
        }

        public class CV2Header
        {
            private static Dictionary<int, PixelFormat> pixelFormatTable
                = new Dictionary<int, PixelFormat>();
            private static Dictionary<int, int> bppTable
                = new Dictionary<int, int>();

            static CV2Header()
            {
                pixelFormatTable.Add(8, PixelFormat.Format8bppIndexed);
                pixelFormatTable.Add(16, PixelFormat.Format16bppRgb565);
                pixelFormatTable.Add(24, PixelFormat.Format32bppArgb);
                pixelFormatTable.Add(32, PixelFormat.Format32bppArgb);

                bppTable.Add(8, 1);
                bppTable.Add(16, 2);
                bppTable.Add(24, 4); // はぁ？
                bppTable.Add(32, 4);
            }

            private int _bpp;
            private int _width;
            private int _height;
            int _pwidth;

            public int bpp
            {
                get { return _bpp; }
                set
                {
                    if (!pixelFormatTable.ContainsKey(value))
                        throw new FormatException("サポートしていない色深度です。");
                    _bpp = value;
                }
            }

            public int Bpp
            {
                get { return bppTable[bpp]; }
            }

            public PixelFormat PixelFormat
            {
                get { return pixelFormatTable[bpp]; }
            }

            public int Width
            {
                get { return _width; }
                set { _width = value; }
            }

            public int Height
            {
                get { return _height; }
                set { _height = value; }
            }

            public int PaddedWidth
            {
                get { return _pwidth; }
                set { _pwidth = value; }
            }

            public void Serialize(Binary output)
            {
                output.WriteByte((byte)bpp);
                output.WriteInt32(Width);
                output.WriteInt32(Height);
                output.WriteInt32(PaddedWidth);
                output.WriteInt32(0);
            }

            public static CV2Header Deserialize(Stream input)
            {
                CV2Header header = new CV2Header();
                Binary reader = new Binary(input);
                header.bpp = reader.ReadByte();
                header.Width = reader.ReadInt32();
                header.Height = reader.ReadInt32();
                header.PaddedWidth = reader.ReadInt32();
                int resd1 = reader.ReadInt32();

                if (header.PaddedWidth < header.Width)
                    throw new FormatException("ヘッダーが不正です。");
                return header;
            }
        }

        static void ProcessCV2(string inputName)
        {
            string outputName = Path.ChangeExtension(inputName, "cv2");
            try
            {
                CV2Reader cv2;
                using (FileStream ofs = new FileStream(outputName, FileMode.Open, FileAccess.Read))
                {
                    cv2 = new CV2Reader(ofs);
                    if (cv2.Header.bpp == 8)
                    {
                        throw new Exception(inputName + " : do NOT support Palette !!");
                    }

                }
                using (FileStream ifs = new FileStream(inputName, FileMode.Open, FileAccess.Read))
                {
                    Bitmap image = cv2.Image;
                    Bitmap inputImage = (Bitmap)Image.FromStream(ifs);
                    using (FileStream ofs = new FileStream(outputName, FileMode.Create, FileAccess.Write))
                    {
                        Binary ob = new Binary(ofs);
                        cv2.Header.Serialize(ob);
                        System.Console.WriteLine(inputName + " --> " + outputName);
                        for (int x = 0; x < cv2.Header.Height; x++)
                            for (int y = 0; y < cv2.Header.PaddedWidth; y++)
                            {
                                if (y < cv2.Header.Width)
                                {
                                    Color c = inputImage.GetPixel(y, x);
                                    switch (cv2.Header.bpp)
                                    {
                                        case 8:
                                            throw new Exception(inputName + " : do NOT support Palette !!");
                                        case 16:
                                            {
                                                short b = (short)((c.R>>3) << 11 | (c.G>>2) << 5 | (c.B>>3));
                                                ob.WriteInt16(b);
                                                break;
                                            }
                                        case 24:
                                            {
                                                int b = 255<<24 | c.R << 16 | c.G << 8 | c.B;
                                                ob.WriteInt32(b);
                                                break;
                                            }
                                        case 32:
                                            {
                                                int b = c.A << 24 | c.R << 16 | c.G << 8 | c.B;
                                                ob.WriteInt32(b);
                                                break;
                                            }
                                        default:
                                            throw new Exception(inputName + " : unknown bpp !!");
                                    }
                                }
                                else
                                {
                                    switch (cv2.Header.bpp)
                                    {
                                        case 8:
                                            throw new Exception(inputName + " : do NOT support Palette !!");
                                        case 16:
                                            ob.WriteInt16(0);
                                            break;
                                        case 24:
                                            ob.WriteInt32(0);
                                            break;
                                        case 32:
                                            ob.WriteInt32(0);
                                            break;
                                        default:
                                            throw new Exception(inputName + " : unknown bpp !!");
                                    }
                                }
                            }
                    }
                }
                File.Delete(inputName);
            }
            catch (Exception e)
            {
                System.Console.Error.Write(e.Message);
            }
        }

        static void ProcessCV2Directory(string dirName)
        {
            SearchOption searchOpt = SearchOption.AllDirectories;
            string[] files = Directory.GetFiles(dirName, "*.png", searchOpt);
            foreach (string fileName in files)
            {
                ProcessCV2(fileName);
            }
        }

        static void Main(string[] args)
        {
            if (args.Length < 1)
            {
                return;
            }
            try
            {
                FileAttributes attr = File.GetAttributes(args[0]);
                if ((attr & FileAttributes.Directory) != 0)
                {
                    ProcessCV2Directory(args[0]);
                }
                else
                {
                    ProcessCV2(args[0]);
                }
            }
            catch (Exception e)
            {
                System.Console.Error.Write(e.Message);
            }
            finally
            {
                System.Console.WriteLine("Press ENTER to continue...");
                System.Console.ReadLine();
            }
        }

        public class Binary
        {
            public Stream BaseStream = null;

            public Binary(Stream s)
            {
                BaseStream = s;
            }

            public void ReadBytes(byte[] buff, int offset, int count)
            {
                BaseStream.Read(buff, offset, count);
            }

            public byte[] ReadBytes(int count)
            {
                byte[] buff = new byte[count];
                BaseStream.Read(buff, 0, count);
                return buff;
            }

            public void WriteBytes(byte[] buff)
            {
                BaseStream.Write(buff, 0, buff.Length);
            }

            public int ReadByte()
            {
                return BaseStream.ReadByte();
            }

            public short ReadInt16()
            {
                return BitConverter.ToInt16(ReadBytesInternal(2), 0);
            }

            public int ReadInt32()
            {
                return BitConverter.ToInt32(ReadBytesInternal(4), 0);
            }

            public ushort ReadUInt16()
            {
                return BitConverter.ToUInt16(ReadBytesInternal(2), 0);
            }

            public uint ReadUInt32()
            {
                return BitConverter.ToUInt32(ReadBytesInternal(4), 0);
            }

            public float ReadSingle()
            {
                return BitConverter.ToSingle(ReadBytesInternal(4), 0);
            }

            public void WriteByte(byte x)
            {
                byte[] b = new byte[1];
                b[0] = x;
                WriteBytesInternal(b);
            }

            public void WriteInt16(short x)
            {
                WriteBytesInternal(BitConverter.GetBytes(x));
            }

            public void WriteInt32(int x)
            {
                WriteBytesInternal(BitConverter.GetBytes(x));
            }

            public void WriteUInt16(ushort x)
            {
                WriteBytesInternal(BitConverter.GetBytes(x));
            }

            public void WriteUInt32(uint x)
            {
                WriteBytesInternal(BitConverter.GetBytes(x));
            }

            public void WriteSingle(float x)
            {
                WriteBytesInternal(BitConverter.GetBytes(x));
            }

            private byte[] ReadBytesInternal(int count)
            {
                byte[] buff = ReadBytes(count);
                //Array.Reverse(buff);
                return buff;
            }

            private void WriteBytesInternal(byte[] buff)
            {
                //Array.Reverse(buff);
                WriteBytes(buff);
            }
        }
    }
}
