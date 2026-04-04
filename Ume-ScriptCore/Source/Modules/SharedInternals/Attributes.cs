using System;
using System.Collections.Generic;

namespace Ume
{
	[VisibleToOtherModules]
	[AttributeUsage(AttributeTargets.Class, Inherited = false)]
	internal sealed class AssetFileNameExtensionAttribute : Attribute
	{
		public string preferredExtension { get; }
		public IEnumerable<string> otherExtensions { get; }

		public AssetFileNameExtensionAttribute(string preferredExtension, params string[] otherExtensions)
		{
			this.preferredExtension = preferredExtension;
			this.otherExtensions = otherExtensions;
		}
	}

	[VisibleToOtherModules]
	[System.AttributeUsage(System.AttributeTargets.Method)]
	internal class ThreadAndSerializationSafeAttribute : System.Attribute
	{
		public ThreadAndSerializationSafeAttribute()
		{
		}
	}


	[System.AttributeUsage(System.AttributeTargets.Struct)]
	[VisibleToOtherModules]
	internal class IL2CPPStructAlignmentAttribute : System.Attribute
	{
		public int Align;
		public IL2CPPStructAlignmentAttribute()
		{
			Align = 1;
		}
	}

	[AttributeUsage(AttributeTargets.Parameter, AllowMultiple = false)]
	[VisibleToOtherModules]
	internal class WritableAttribute : Attribute
	{
	}

	[AttributeUsage(AttributeTargets.Class)]
	[VisibleToOtherModules]
	internal class RejectDragAndDropMaterial : Attribute
	{
	}

	[AttributeUsage(AttributeTargets.Assembly)]
	[VisibleToOtherModules]
	internal class UnityEngineModuleAssembly : Attribute
	{
	}

	[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, Inherited = false)]
	[VisibleToOtherModules]
	internal sealed class NativeClassAttribute : Attribute
	{
		public string QualifiedNativeName { get; private set; }
		public string Declaration { get; private set; }

		public NativeClassAttribute(string qualifiedCppName)
		{
			QualifiedNativeName = qualifiedCppName;
			Declaration = "class " + qualifiedCppName;
		}

		public NativeClassAttribute(string qualifiedCppName, string declaration)
		{
			QualifiedNativeName = qualifiedCppName;
			Declaration = declaration;
		}
	}
}
