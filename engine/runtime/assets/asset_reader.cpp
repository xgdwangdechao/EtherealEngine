#include "asset_reader.h"
#include "../rendering/texture.h"
#include "../rendering/uniform.h"
#include "../rendering/shader.h"
#include "../rendering/mesh.h"
#include "../rendering/material.h"
#include "../rendering/vertex_buffer.h"
#include "../rendering/index_buffer.h"
#include "../ecs/prefab.h"
#include "core/subsystem/tasks.hpp"
#include "core/filesystem/filesystem.h"
#include "core/serialization/serialization.h"
#include "core/serialization/binary_archive.h"
#include "core/serialization/associative_archive.h"
#include "core/serialization/types/map.hpp"
#include "core/serialization/types/vector.hpp"
#include "../meta/rendering/material.hpp"
#include "../meta/rendering/mesh.hpp"
#include <cstdint>

namespace runtime
{

	template<>
	core::task_future<asset_handle<texture>> asset_reader::load_from_file<texture>(const std::string& key, const load_mode& mode, asset_handle<texture> original)
	{
		fs::path absolute_key = fs::absolute(fs::resolve_protocol(key).string());
		auto compiled_absolute_key = absolute_key.string() + extensions::get_compiled_format<texture>();
		auto read_memory = std::make_shared<fs::byte_array_t>();

		auto read_memory_func = [read_memory, compiled_absolute_key]()
		{
			if (!read_memory)
				return false;

			auto stream = std::ifstream{ compiled_absolute_key, std::ios::in | std::ios::binary };
			*read_memory = fs::read_stream(stream);

			return true;
		};

		auto create_resource_func = [result = original, read_memory, key](bool read_result) mutable
		{
			// if someone destroyed our memory
			if (!read_memory)
				return result;
			// if nothing was read
			if (read_memory->empty())
				return result;

			const gfx::Memory* mem = gfx::copy(read_memory->data(), static_cast<std::uint32_t>(read_memory->size()));
			read_memory->clear();
			read_memory.reset();

			
			if (nullptr != mem)
			{
				auto tex = std::make_shared<texture>(mem, 0, 0, nullptr);
				result.link->id = key;
				result.link->asset = tex;
			}
			return result;
		};


		auto& ts = core::get_subsystem<core::task_system>();

		if (mode == load_mode::async)
		{
			auto ready_memory_task = ts.push_ready(read_memory_func);
			auto create_resource_task = ts.push_awaitable_on_main(create_resource_func, ready_memory_task);
			return create_resource_task;
		}
		else
		{
			auto ready_memory_task = ts.push_ready_on_main(read_memory_func);
			auto create_resource_task = ts.push_awaitable_on_main(create_resource_func, ready_memory_task);
			return create_resource_task;
		}

	}

	template<>
	core::task_future<asset_handle<shader>> asset_reader::load_from_file<shader>(const std::string& key, const load_mode& mode, asset_handle<shader> original)
	{
		fs::path absolute_key = fs::absolute(fs::resolve_protocol(key).string());
		auto compiled_absolute_key = absolute_key.string() + extensions::get_compiled_format<shader>();
		auto read_memory = std::make_shared<fs::byte_array_t>();

		auto read_memory_func = [read_memory, compiled_absolute_key]()
		{
			if (!read_memory)
				return false;

			auto stream = std::ifstream{ compiled_absolute_key, std::ios::in | std::ios::binary };
			*read_memory = fs::read_stream(stream);

			return true;
		};

		auto create_resource_func = [result = original, read_memory, key](bool read_result) mutable
		{
			// if someone destroyed our memory
			if (!read_memory)
				return result;
			// if nothing was read
			if (read_memory->empty())
				return result;

			const gfx::Memory* mem = gfx::copy(read_memory->data(), static_cast<std::uint32_t>(read_memory->size()));
			read_memory->clear();
			read_memory.reset();

			if (nullptr != mem)
			{
				auto shdr = std::make_shared<shader>();
				shdr->populate(mem);
				auto uniform_count = gfx::getShaderUniforms(shdr->handle);
				if (uniform_count > 0)
				{
					std::vector<gfx::UniformHandle> uniforms(uniform_count);
					gfx::getShaderUniforms(shdr->handle, &uniforms[0], uniform_count);
					shdr->uniforms.reserve(uniform_count);
					for (auto& uni : uniforms)
					{
						auto hUniform = std::make_shared<uniform>();
						hUniform->populate(uni);

						shdr->uniforms.push_back(hUniform);
					}
				}
				result.link->id = key;
				result.link->asset = shdr;
			}

			return result;
		};

		auto& ts = core::get_subsystem<core::task_system>();

		if (mode == load_mode::async)
		{
			auto ready_memory_task = ts.push_ready(read_memory_func);
			auto create_resource_task = ts.push_awaitable_on_main(create_resource_func, ready_memory_task);
			return create_resource_task;
		}
		else
		{
			auto ready_memory_task = ts.push_ready_on_main(read_memory_func);
			auto create_resource_task = ts.push_awaitable_on_main(create_resource_func, ready_memory_task);
			return create_resource_task;
		}
	}

	template<>
	core::task_future<asset_handle<mesh>> asset_reader::load_from_file<mesh>(const std::string& key, const load_mode& mode, asset_handle<mesh> original)
	{
		fs::path absolute_key = fs::absolute(fs::resolve_protocol(key).string());
		auto compiled_absolute_key = absolute_key.string() + extensions::get_compiled_format<mesh>();
        
		struct wrapper_t
		{
            std::shared_ptr<::mesh> mesh;
		};

        auto wrapper = std::make_shared<wrapper_t>();
		wrapper->mesh = std::make_shared<mesh>();
		auto read_memory_func = [wrapper, compiled_absolute_key]() mutable
		{
			mesh::load_data data;
			{
				std::ifstream stream{ compiled_absolute_key, std::ios::in | std::ios::binary };
				cereal::iarchive_binary_t ar(stream);

				try_load(ar, cereal::make_nvp("mesh", data));
			}
			wrapper->mesh->prepare_mesh(data.vertex_format);
			wrapper->mesh->set_vertex_source(&data.vertex_data[0], data.vertex_count, data.vertex_format);
			wrapper->mesh->add_primitives(data.triangle_data);
			wrapper->mesh->bind_skin(data.skin_data);
			wrapper->mesh->bind_armature(data.root_node);
			wrapper->mesh->end_prepare(true, false, false, false);

			return true;
		};

		auto create_resource_func = [result = original, wrapper, key](bool read_result) mutable
		{
			// Build the mesh
			wrapper->mesh->build_vb();
			wrapper->mesh->build_ib();

			if (wrapper->mesh->get_status() == mesh_status::prepared)
			{
				result.link->id = key;
				result.link->asset = wrapper->mesh;
			}
			wrapper.reset();

			return result;
		};

		auto& ts = core::get_subsystem<core::task_system>();

		if (mode == load_mode::async)
		{
			auto ready_memory_task = ts.push_ready(read_memory_func);
			auto create_resource_task = ts.push_awaitable_on_main(create_resource_func, ready_memory_task);
			return create_resource_task;
		}
		else
		{
			auto ready_memory_task = ts.push_ready_on_main(read_memory_func);
			auto create_resource_task = ts.push_awaitable_on_main(create_resource_func, ready_memory_task);
			return create_resource_task;
		}
	}

	template<>
	core::task_future<asset_handle<material>> asset_reader::load_from_file<material>(const std::string& key, const load_mode& mode, asset_handle<material> original)
	{
		fs::path absolute_key = fs::absolute(fs::resolve_protocol(key).string());
		auto compiled_absolute_key = absolute_key.string() + extensions::get_compiled_format<material>();
        
		struct wrapper_t
		{
            std::shared_ptr<::material> material;
		};

        auto wrapper = std::make_shared<wrapper_t>();
		wrapper->material = std::make_shared<material>();
		auto read_memory_func = [wrapper, compiled_absolute_key]() mutable
		{
			std::ifstream stream{ compiled_absolute_key, std::ios::in | std::ios::binary };
			cereal::iarchive_associative_t ar(stream);

			try_load(ar, cereal::make_nvp("material", wrapper->material));

			return true;
		};

		auto create_resource_func = [result = original, wrapper, key](bool read_result) mutable
		{
			result.link->id = key;
			result.link->asset = wrapper->material;
			wrapper.reset();

			return result;
		};

		auto& ts = core::get_subsystem<core::task_system>();

		if (mode == load_mode::async)
		{
			auto ready_memory_task = ts.push_ready(read_memory_func);
			auto create_resource_task = ts.push_awaitable_on_main(create_resource_func, ready_memory_task);
			return create_resource_task;
		}
		else
		{
			auto ready_memory_task = ts.push_ready_on_main(read_memory_func);
			auto create_resource_task = ts.push_awaitable_on_main(create_resource_func, ready_memory_task);
			return create_resource_task;
		}
	}

	template<>
	core::task_future<asset_handle<prefab>> asset_reader::load_from_file<prefab>(const std::string& key, const load_mode& mode, asset_handle<prefab> original)
	{
		fs::path absolute_key = fs::absolute(fs::resolve_protocol(key).string());
		auto compiled_absolute_key = absolute_key.string() + extensions::get_compiled_format<prefab>();

		std::shared_ptr<std::istringstream> read_memory = std::make_shared<std::istringstream>();

		auto read_memory_func = [read_memory, compiled_absolute_key]()
		{
			if (!read_memory)
				return false;

			auto stream = std::fstream{ compiled_absolute_key, std::fstream::in | std::fstream::out | std::ios::binary };
			auto mem = fs::read_stream(stream);
			*read_memory = std::istringstream(std::string(mem.data(), mem.size()));
			
			return true;
		};

		auto create_resource_func = [result = original, read_memory, key](bool read_result) mutable
		{
			auto pfab = std::make_shared<prefab>();
			pfab->data = read_memory;

			result.link->id = key;
			result.link->asset = pfab;

			return result;
		};


		auto& ts = core::get_subsystem<core::task_system>();

		if (mode == load_mode::async)
		{
			auto ready_memory_task = ts.push_ready(read_memory_func);
			auto create_resource_task = ts.push_awaitable_on_main(create_resource_func, ready_memory_task);
			return create_resource_task;
		}
		else
		{
			auto ready_memory_task = ts.push_ready_on_main(read_memory_func);
			auto create_resource_task = ts.push_awaitable_on_main(create_resource_func, ready_memory_task);
			return create_resource_task;
		}
	}

	template<>
	core::task_future<asset_handle<scene>> asset_reader::load_from_file<scene>(const std::string& key, const load_mode& mode, asset_handle<scene> original)
	{
		fs::path absolute_key = fs::absolute(fs::resolve_protocol(key).string());
		auto compiled_absolute_key = absolute_key.string() + extensions::get_compiled_format<scene>();

		std::shared_ptr<std::istringstream> read_memory = std::make_shared<std::istringstream>();

		auto read_memory_func = [read_memory, compiled_absolute_key]()
		{
			if (!read_memory)
				return false;

			auto stream = std::fstream{ compiled_absolute_key, std::fstream::in | std::fstream::out | std::ios::binary };
			auto mem = fs::read_stream(stream);
			*read_memory = std::istringstream(std::string(mem.data(), mem.size()));
		
			return true;
		};

		auto create_resource_func = [result = original, read_memory, key](bool read_result) mutable
		{
			auto sc = std::make_shared<scene>();
			sc->data = read_memory;

			result.link->id = key;
			result.link->asset = sc;

			return result;
		};


		auto& ts = core::get_subsystem<core::task_system>();

		if (mode == load_mode::async)
		{
			auto ready_memory_task = ts.push_ready(read_memory_func);
			auto create_resource_task = ts.push_awaitable_on_main(create_resource_func, ready_memory_task);
			return create_resource_task;
		}
		else
		{
			auto ready_memory_task = ts.push_ready_on_main(read_memory_func);
			auto create_resource_task = ts.push_awaitable_on_main(create_resource_func, ready_memory_task);
			return create_resource_task;
		}
	}

	template<>
	core::task_future<asset_handle<shader>> asset_reader::load_from_memory<shader>(const std::string& key, const std::uint8_t* data, std::uint32_t size)
	{

		auto create_resource_func = [&key, data, size]() mutable
		{
			asset_handle<shader> result;
			// if nothing was read
			if (!data && size == 0)
				return result;
			const gfx::Memory* mem = gfx::copy(data, size);
			if (nullptr != mem)
			{
				auto shdr = std::make_shared<shader>();
				shdr->populate(mem);
				auto uniform_count = gfx::getShaderUniforms(shdr->handle);
				if (uniform_count > 0)
				{
					std::vector<gfx::UniformHandle> uniforms(uniform_count);
					gfx::getShaderUniforms(shdr->handle, &uniforms[0], uniform_count);
					shdr->uniforms.reserve(uniform_count);
					for (auto& uni : uniforms)
					{
						auto hUniform = std::make_shared<uniform>();
						hUniform->populate(uni);

						shdr->uniforms.push_back(hUniform);
					}
				}

				result.link->id = key;
				result.link->asset = shdr;
			}
			return result;
		};

		auto& ts = core::get_subsystem<core::task_system>();
		auto create_resource_task = ts.push_ready_on_main(create_resource_func);
		return create_resource_task;

	}

}