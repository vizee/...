using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace LibDotJson
{
	public class DotJson
	{
		private JNode m_root;

		public DotJson()
		{
			m_root = null;
		}

		public JNode Root
		{
			get { return m_root; }
			set { m_root = value; }
		}

		public bool Parse(Stream stream, Encoding encoding)
		{
			byte[] bytes = new byte[stream.Length - stream.Position];
			if (stream.Read(bytes, 0, bytes.Length) > 0)
			{
				JParser parser = new JParser();
				char[] json = encoding.GetChars(bytes);
				int start = 0;
				if (encoding.CodePage == 65001 && json[0] == 0xfeff)
				{
					start = 1;
				}
				return (parser.Parse(json, start, out m_root) != -1);
			}
			return false;
		}

		public bool Parse(string json)
		{
			if (json.Length == 0)
			{
				return false;
			}
			JParser parser = new JParser();
			return (parser.Parse(json.ToCharArray(), 0, out m_root) != -1);
		}

		public class JParser
		{
			private char[] m_json;
			private int m_length;

			public int Parse(char[] json, int start, out JNode node)
			{
				m_json = json;
				m_length = m_json.Length;
				return parseNode(start, out node);
			}

			private int parseNode(int start, out JNode node)
			{
				node = null;
				int result = 0;
				switch (m_json[start])
				{
				case '{':
					result = parseObject(start, out node);
					break;
				case '[':
					result = parseArray(start, out node);
					break;
				case '"':
					result = parseString(start, out node);
					break;
				case 'N':
				case 'n':
					if (Char.ToUpper(m_json[start + 1]) == 'U'
						&& Char.ToUpper(m_json[start + 2]) == 'L'
						&& Char.ToUpper(m_json[start + 3]) == 'L')
					{
						node = new JNode(0);
						result = 4;
					}
					break;
				case 'F':
				case 'f':
					if (Char.ToUpper(m_json[start + 1]) == 'A'
						&& Char.ToUpper(m_json[start + 2]) == 'L'
						&& Char.ToUpper(m_json[start + 3]) == 'S'
						&& Char.ToUpper(m_json[start + 4]) == 'E')
					{
						node = new JNode(1);
						result = 5;
					}
					break;
				case 'T':
				case 't':
					if (Char.ToUpper(m_json[start + 1]) == 'R'
						&& Char.ToUpper(m_json[start + 2]) == 'U'
						&& Char.ToUpper(m_json[start + 3]) == 'E')
					{
						node = new JNode(2);
						result = 4;
					}
					break;
				default:
					if (Char.IsWhiteSpace(m_json[start]))
					{
						int p = start + 1;
						while (Char.IsWhiteSpace(m_json[p]))
						{
							p++;
						}
						return p - start;
					}
					else if (Char.IsDigit(m_json[start]) || m_json[start] == '-')
					{
						result = parseNumber(start, out node);
					}
					break;
				}
				return result;
			}

			private int parseNumber(int start, out JNode node)
			{
				node = null;
				bool frac = false;
				bool exp = false;
				int p = start;
				if (m_json[p] == '-')
				{
					p++;
				}
				while (p < m_length)
				{
					if (m_json[p] == '.')
					{
						if (frac)
						{
							return -1;
						}
						else
						{
							frac = true;
						}
					}
					else if (m_json[p] == 'E' || m_json[p] == 'e')
					{
						if (exp)
						{
							return -1;
						}
						else
						{
							if (m_json[p + 1] == '+' || m_json[p + 1] == '-')
							{
								p++;
							}
							exp = true;
						}
					}
					else if (!Char.IsDigit(m_json[p]))
					{
						break;
					}
					p++;
				}
				int length = p - start;
				var number = new string(m_json, start, length);
				int intValue = 0;
				if (Int32.TryParse(number, out intValue))
				{
					node = new JValue<int>(3, intValue);
				}
				else
				{
					float floatValue = 0.0f;
					if (Single.TryParse(number, out floatValue))
					{
						node = new JValue<float>(4, floatValue);
					}
					else
					{
						return -1;
					}
				}
				return length;
			}

			private int parseString(int start, out JNode node)
			{
				node = null;
				if (m_json[start] != '"')
				{
					return -1;
				}
				StringBuilder builder = new StringBuilder(5);
				int p = start + 1;
				while (p < m_length && m_json[p] != '"')
				{
					switch (m_json[p])
					{
					case '\\':
						char escape = default(char);
						int length = 0;
						switch (m_json[p + 1])
						{
						case '"':
							escape = '"';
							length = 1;
							break;
						case '\\':
							escape = '\\';
							length = 1;
							break;
						case '/':
							escape = '/';
							length = 1;
							break;
						case 'b':
							escape = '\b';
							length = 1;
							break;
						case 'f':
							escape = '\f';
							length = 1;
							break;
						case 'n':
							escape = '\n';
							length = 1;
							break;
						case 'r':
							escape = '\r';
							length = 1;
							break;
						case 't':
							escape = '\t';
							length = 1;
							break;
						case 'u':
							uint u = 0;
							length = 1;
							for (int i = 0; i < 4; i++)
							{
								char c = m_json[p + 2 + i];
								u <<= 4;
								if (c >= '0' && c <= '9')
								{
									u |= (Convert.ToUInt32(c) & 0x0000000f);
								}
								else if ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
								{
									u |= ((Convert.ToUInt32(c) & 0x0000000f) + 9);
								}
								else
								{
									length = 0;
									break;
								}
								length++;
							}
							if (length > 0)
							{
								escape = Convert.ToChar(u);
							}
							break;
						}
						if (length == 0)
						{
							return -1;
						}
						builder.Append(escape);
						p += length;
						break;
					default:
						builder.Append(m_json[p]);
						break;
					}
					p++;
				}
				if (p == m_length)
				{
					return -1;
				}
				int l = p - start + 1;
				node = new JValue<string>(5, builder.ToString());
				return l;
			}

			private int parseArray(int start, out JNode node)
			{
				node = null;
				if (m_json[start] != '[')
				{
					return -1;
				}
				JNode jnode = null;
				var jarray = new JArray();
				int step = 0;
				int length = 1;
				int p = start + 1;
				while (length > 0 && p < m_length)
				{
					if (m_json[p] == ',')
					{
						if (step == 2)
						{
							p++;
							step = 1;
						}
						else
						{
							return -1;
						}
					}
					length = parseNode(p, out jnode);
					if (jnode != null)
					{
						switch (step)
						{
						case 0:
						case 1:
							jarray.Add(jnode);
							step = 2;
							break;
						}
					}
					p += length;
				}
				if (length == 0 && m_json[p] == ']' && (step == 2 || step == 0))
				{
					node = jarray;
					return p - start + 1;
				}
				return -1;
			}

			private int parseObject(int start, out JNode node)
			{
				node = null;
				if (m_json[start] != '{')
				{
					return -1;
				}
				JNode jnode = null;
				string key = null;
				var jobject = new JObject();
				int step = 0;
				int length = 1;
				int p = start + 1;
				while (length > 0 && p < m_length)
				{
					if (m_json[p] == ':')
					{
						if (step == 2)
						{
							p++;
							step = 3;
						}
						else
						{
							return -1;
						}
					}
					else if (m_json[p] == ',')
					{
						if (step == 4)
						{
							p++;
							step = 1;
						}
						else
						{
							return -1;
						}
					}
					length = parseNode(p, out jnode);
					if (jnode != null)
					{
						switch (step)
						{
						case 0:
						case 1:
							if (jnode.Type != 5)
							{
								return -1;
							}
							key = ((JValue<string>)jnode).Value;
							step = 2;
							break;
						case 3:
							jobject[key] = jnode;
							step = 4;
							break;
						}
					}
					p += length;
				}
				if (length == 0 && m_json[p] == '}' && (step == 4 || step == 0))
				{
					node = jobject;
					return p - start + 1;
				}
				return -1;
			}
		}

		public class JNode
		{
			/// <summary>
			/// 0 Null
			/// 1 False
			/// 2 True
			/// 3 Integer
			/// 4 Float
			/// 5 String
			/// 6 Array
			/// 7 Object
			/// </summary> 
			public int Type
			{
				get;
				private set;
			}

			public JNode(int type)
			{
				Type = type;
			}

			public override string ToString()
			{
				switch (Type)
				{
				case 0:
					return "null";
				case 1:
					return "false";
				case 2:
					return "true";
				}
				return "undefined";
			}
		}

		public class JValue<T> : JNode
		{
			public T Value;

			public JValue(int type, T value)
				: base(type)
			{
				Value = value;
			}

			public override string ToString()
			{
				switch (Type)
				{
				case 3:
					return Convert.ToString(((JValue<int>)((JNode)this)).Value);
				case 4:
					return Convert.ToString(((JValue<float>)((JNode)this)).Value);
				case 5:
					return "\"" + ((JValue<string>)((JNode)this)).Value + "\"";
				default:
					break;
				}
				return Value.ToString();
			}
		}

		public class JArray : JNode
		{
			private List<JNode> m_values;

			public JArray()
				: base(6)
			{
				m_values = new List<JNode>();
			}

			public JNode this[int index]
			{
				get
				{
					return m_values[index];
				}
				set
				{
					m_values[index] = value;
				}
			}

			public int Count
			{
				get { return m_values.Count; }
			}

			public void Add(JNode value)
			{
				m_values.Add(value);
			}

			public void Remove(JNode value)
			{
				m_values.Remove(value);
			}

			public void Clear()
			{
				m_values.Clear();
			}

			public override string ToString()
			{
				var builder = new StringBuilder();
				builder.Append('[');
				for (int i = 0; i < m_values.Count; i++)
				{
					builder.Append(m_values[i].ToString());
					if (i < m_values.Count - 1)
					{
						builder.Append(',');
					}
				}
				builder.Append(']');
				return builder.ToString();
			}
		}

		public class JObject : JNode
		{
			private List<JObjectPair> m_pairs;

			public JObject()
				: base(7)
			{
				m_pairs = new List<JObjectPair>();
			}

			public JNode this[string key]
			{
				get
				{
					int i = find(key);
					if (i != -1)
					{
						return m_pairs[i].Value;
					}
					return null;
				}
				set
				{
					int i = find(key);
					if (i != -1)
					{
						m_pairs[i].Value = value;
					}
					else
					{
						var pair = new JObjectPair();
						pair.Key = key;
						pair.Value = value;
						m_pairs.Add(pair);
					}
				}
			}

			public void Remove(string key)
			{
				int i = find(key);
				if (i != -1)
				{
					m_pairs.RemoveAt(i);
				}
			}

			private int find(string key)
			{
				for (int i = 0; i < m_pairs.Count; i++)
				{
					if (String.Equals(m_pairs[i].Key, key))
					{
						return i;
					}
				}
				return -1;
			}

			public override string ToString()
			{
				var builder = new StringBuilder();
				builder.Append('{');
				for (int i = 0; i < m_pairs.Count; i++)
				{
					builder.Append('"');
					builder.Append(m_pairs[i].Key);
					builder.Append('"');
					builder.Append(':');
					builder.Append(m_pairs[i].Value.ToString());
					if (i < m_pairs.Count - 1)
					{
						builder.Append(',');
					}
				}
				builder.Append('}');
				return builder.ToString();
			}
		}

		private class JObjectPair
		{
			public string Key;
			public JNode Value;
		}
	}
}
