#include "../../dxbc/dxbc_api.h"

#include "../test_common.h"

namespace dxbc_spv::tests::dxbc {

using namespace dxbc_spv::dxbc;

/* Minimal VS/PS pair compiled with D3DCompile (shader model 5.0):
 * struct V { float4 pos : SV_Position; min10float4 color : COLOR0; };
 * V vs(uint id : SV_VertexID) {
 *   V v;
 *   v.pos = float4(id == 2 ? 3 : -1, id == 1 ? 3 : -1, 0, 1);
 *   v.color = min10float4(0.25, 0.5, 0.75, 1);
 *   return v;
 * }
 * float4 ps(V v) : SV_Target { return v.color; }
 * The D3D11 UMD reconstructs the COLOR0 semantic as ATTRIB4. */
static const uint32_t min10Vs[] = {
  0x43425844, 0x54444c79, 0xdc39dc72, 0x8480ffce, 0x155a7bf1, 0x00000001, 0x000001d8, 0x00000003,
  0x0000002c, 0x00000068, 0x000000cc, 0x31475349, 0x00000034, 0x00000001, 0x00000008, 0x00000000,
  0x00000028, 0x00000000, 0x00000006, 0x00000001, 0x00000000, 0x00000101, 0x00000000, 0x565f5653,
  0x65747265, 0x00444978, 0x3147534f, 0x0000005c, 0x00000002, 0x00000008, 0x00000000, 0x00000048,
  0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x00000000, 0x00000000, 0x00000054,
  0x00000004, 0x00000000, 0x00000003, 0x00000001, 0x0000000f, 0x00000002, 0x505f5653, 0x7469736f,
  0x006e6f69, 0x52545441, 0x00004249, 0x58454853, 0x00000104, 0x00010050, 0x00000041, 0x0101086a,
  0x04000060, 0x00101012, 0x00000000, 0x00000006, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
  0x04000065, 0x801020f2, 0x00008001, 0x00000001, 0x02000068, 0x00000001, 0x0a000020, 0x00100032,
  0x00000000, 0x00101006, 0x00000000, 0x00004002, 0x00000002, 0x00000001, 0x00000000, 0x00000000,
  0x0f000037, 0x00100032, 0x00000000, 0x00100046, 0x00000000, 0x00004002, 0x00000003, 0x00000003,
  0x00000000, 0x00000000, 0x00004002, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x0500002b,
  0x00102032, 0x00000000, 0x00100046, 0x00000000, 0x08000036, 0x001020c2, 0x00000000, 0x00004002,
  0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x09000036, 0x801020f2, 0x00008001, 0x00000001,
  0x00004002, 0x3e800000, 0x3f000000, 0x3f400000, 0x3f800000, 0x0100003e,
};

static const uint32_t min10Ps[] = {
  0x43425844, 0xefd9a2c6, 0xe860d1a0, 0xac1e866c, 0xdd713840, 0x00000001, 0x00000118, 0x00000003,
  0x0000002c, 0x00000090, 0x000000cc, 0x31475349, 0x0000005c, 0x00000002, 0x00000008, 0x00000000,
  0x00000048, 0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x00000f0f, 0x00000000, 0x00000000,
  0x00000054, 0x00000004, 0x00000000, 0x00000003, 0x00000001, 0x00000f0f, 0x00000002, 0x505f5653,
  0x7469736f, 0x006e6f69, 0x52545441, 0x00004249, 0x3147534f, 0x00000034, 0x00000001, 0x00000008,
  0x00000000, 0x00000028, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x0000000f, 0x00000000,
  0x545f5653, 0x65677261, 0x00000074, 0x58454853, 0x00000044, 0x00000050, 0x00000011, 0x0101086a,
  0x04001062, 0x801010f2, 0x00008001, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x06000036,
  0x001020f2, 0x00000000, 0x80101e46, 0x00008001, 0x00000001, 0x0100003e,
};

void testDxbcMinPrecisionIo() {
  const std::pair<const uint32_t*, size_t> shaders[] = {
    { min10Vs, sizeof(min10Vs) },
    { min10Ps, sizeof(min10Ps) },
  };

  for (const auto& shader : shaders) {
    /* Native 16-bit arithmetic must not change the shader interface width. */
    for (bool enableFloat16 : { false, true }) {
      ir::CompileOptions options;
      options.min16Options.enableFloat16 = enableFloat16;

      auto builder = compileShaderToLegalizedIr(shader.first, shader.second, { }, options);
      ok(builder.has_value());

      if (!builder)
        continue;

      uint32_t colorInterfaces = 0u;

      for (const auto& op : *builder) {
        if (op.getOpCode() != ir::OpCode::eDclInput &&
            op.getOpCode() != ir::OpCode::eDclOutput)
          continue;

        if (uint32_t(op.getOperand(1u)) != 1u)
          continue;

        ok(op.getType() == ir::Type(ir::ScalarType::eF32, 4u));
        colorInterfaces++;
      }

      ok(colorInterfaces == 1u);
    }
  }
}

}
