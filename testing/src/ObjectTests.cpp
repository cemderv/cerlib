// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "cerlib/details/ObjectMacros.hpp"
#include "util/Object.hpp"
#include <cerlib/Formatters.hpp>
#include <cerlib/List.hpp>
#include <cerlib/Util2.hpp>
#include <memory>
#include <snitch/snitch.hpp>
#include <string>

static auto s_info_list = cer::List<std::string>{};

namespace details
{
class AnimalImpl : public cer::details::Object
{
  public:
    explicit AnimalImpl(int id)
        : m_id(id)
    {
        s_info_list.push_back(cer_fmt::format("AnimalImpl({})", m_id));
    }

    ~AnimalImpl() noexcept override
    {
        s_info_list.push_back(cer_fmt::format("~AnimalImpl({})", m_id));
    }

    auto animal_id() const -> int
    {
        return m_id;
    }

  private:
    int m_id;
};
} // namespace details

class Animal
{
    CERLIB_DECLARE_OBJECT(Animal);

  public:
    explicit Animal(int id)
        : m_impl(nullptr)
    {
        set_impl(*this, std::make_unique<details::AnimalImpl>(id).release());
    }

    auto animal_id() const -> int
    {
        return m_impl->animal_id();
    }
};

namespace details
{
class DogImpl : public AnimalImpl
{
  public:
    explicit DogImpl(int base_id, int dog_id)
        : AnimalImpl(base_id)
        , m_dog_id(dog_id)
    {
        s_info_list.push_back(cer_fmt::format("DogImpl({},{})", animal_id(), m_dog_id));
    }

    ~DogImpl() noexcept override
    {
        s_info_list.push_back(cer_fmt::format("~DogImpl({},{})", animal_id(), m_dog_id));
    }

    auto dog_id() const -> int
    {
        return m_dog_id;
    }

  private:
    int m_dog_id;
};
} // namespace details

class Dog : public Animal
{
    CERLIB_DECLARE_DERIVED_OBJECT(Animal, Dog);

    explicit Dog(int base_id, int dog_id)
    {
        set_impl(*this, std::make_unique<details::DogImpl>(base_id, dog_id).release());
    }

    auto dog_id() const -> int
    {
        const auto impl = static_cast<details::DogImpl*>(this->impl());
        return impl->dog_id();
    }
};

namespace details
{
class AnimalHolderImpl : public cer::details::Object
{
  public:
    explicit AnimalHolderImpl(Animal child)
        : child(std::move(child))
    {
        s_info_list.emplace_back("AnimalHolderImpl()");
    }

    ~AnimalHolderImpl() noexcept override
    {
        s_info_list.emplace_back("~AnimalHolderImpl()");
    }

    Animal child;
};
} // namespace details

class AnimalHolder
{
    CERLIB_DECLARE_OBJECT(AnimalHolder);

  public:
    explicit AnimalHolder(Animal child)
        : m_impl(nullptr)
    {
        set_impl(*this, std::make_unique<details::AnimalHolderImpl>(std::move(child)).release());
    }

    auto child() const -> Animal
    {
        return m_impl->child;
    }

    void set_child(const Animal& value)
    {
        m_impl->child = value;
    }
};

CERLIB_IMPLEMENT_OBJECT(Animal);
CERLIB_IMPLEMENT_DERIVED_OBJECT(Animal, Dog);
CERLIB_IMPLEMENT_OBJECT(AnimalHolder);

TEST_CASE("Object")
{
    SECTION("Impl construction")
    {
        auto impl = new details::AnimalImpl(0);

        REQUIRE(s_info_list.size() == 1u);
        REQUIRE(s_info_list.at(0) == "AnimalImpl(0)");

        REQUIRE(impl->ref_count() == 0u);
        impl->add_ref();
        REQUIRE(impl->ref_count() == 1u);
        impl->add_ref();
        REQUIRE(impl->ref_count() == 2u);
        impl->release();
        REQUIRE(impl->ref_count() == 1u);
        impl->release();

        REQUIRE(s_info_list.size() == 2u);
        REQUIRE(s_info_list.at(1) == "~AnimalImpl(0)");
    }

    s_info_list.clear();

    SECTION("Wrapper object construction")
    {
        auto an = Animal{};

        REQUIRE(!an);
        REQUIRE(an.impl() == nullptr);

        REQUIRE(s_info_list.empty());

        an = Animal{1};
        REQUIRE(an);
        REQUIRE(an.animal_id() != 0);
        REQUIRE(an.impl() != nullptr);
        REQUIRE(an.animal_id() == 1);

        REQUIRE(s_info_list.size() == 1u);
        REQUIRE(s_info_list.at(0) == "AnimalImpl(1)");

        {
            const auto impl = an.impl();
            REQUIRE(impl != nullptr);
            REQUIRE(impl == an.impl());
            REQUIRE(impl->animal_id() == an.animal_id());
            REQUIRE(impl->ref_count() == 1u);
        }

        an = {};
        REQUIRE(!an);
        REQUIRE(an.impl() == nullptr);

        REQUIRE(s_info_list.size() == 2u);
        REQUIRE(s_info_list.at(1) == "~AnimalImpl(1)");

        an = Animal{2};
        REQUIRE(an);
        REQUIRE(an.animal_id() != 0);
        REQUIRE(an.impl() != nullptr);
        REQUIRE(an.animal_id() == 2);

        REQUIRE(s_info_list.size() == 3u);
        REQUIRE(s_info_list.at(2) == "AnimalImpl(2)");

        an = Animal{3};
        REQUIRE(s_info_list.size() == 5u);
        REQUIRE(s_info_list.at(3) == "AnimalImpl(3)");
        REQUIRE(s_info_list.at(4) == "~AnimalImpl(2)");
    }

    s_info_list.clear();

    SECTION("Shared references")
    {
        auto an1 = Animal{1};
        auto an2 = an1;

        REQUIRE(s_info_list.size() == 1u);
        REQUIRE(s_info_list.at(0) == "AnimalImpl(1)");

        REQUIRE(an1 == an2);
        REQUIRE(an1);
        REQUIRE(an1.animal_id() == an2.animal_id());
        REQUIRE(an1.impl() == an2.impl());

        const auto impl = an1.impl();
        REQUIRE(impl->ref_count() == 2u);

        auto an3 = an1;
        REQUIRE(impl->ref_count() == 3u);

        an1 = {};
        REQUIRE(impl->ref_count() == 2u);

        an2 = {};
        REQUIRE(impl->ref_count() == 1u);

        an3 = {};
        REQUIRE(s_info_list.size() == 2u);
        REQUIRE(s_info_list.at(1) == "~AnimalImpl(1)");
    }

    s_info_list.clear();

    SECTION("inheritance")
    {
        auto d = Dog{};
        REQUIRE(!d);

        d = Dog{1, 2};
        REQUIRE(d);
        REQUIRE(d.animal_id() == 1);
        REQUIRE(d.dog_id() == 2);

        REQUIRE(s_info_list.size() == 2u);
        REQUIRE(s_info_list.at(0) == "AnimalImpl(1)");
        REQUIRE(s_info_list.at(1) == "DogImpl(1,2)");

        const auto impl = d.impl();
        REQUIRE(impl->ref_count() == 1u);
        REQUIRE(d.animal_id() == 1);
        REQUIRE(d.impl() == impl);

        d = {};
        REQUIRE(!d);
        REQUIRE(d.impl() == nullptr);

        REQUIRE(s_info_list.size() == 4u);
        REQUIRE(s_info_list.at(2) == "~DogImpl(1,2)");
        REQUIRE(s_info_list.at(3) == "~AnimalImpl(1)");
    }

    s_info_list.clear();

    SECTION("Object holder")
    {
        auto holder = AnimalHolder{Dog{1, 2}};

        REQUIRE(s_info_list.size() == 3u);
        REQUIRE(s_info_list.at(0) == "AnimalImpl(1)");
        REQUIRE(s_info_list.at(1) == "DogImpl(1,2)");
        REQUIRE(s_info_list.at(2) == "AnimalHolderImpl()");

        holder.set_child(Dog{3, 4});

        REQUIRE(s_info_list.size() == 7u);
        REQUIRE(s_info_list.at(3) == "AnimalImpl(3)");
        REQUIRE(s_info_list.at(4) == "DogImpl(3,4)");
        REQUIRE(s_info_list.at(5) == "~DogImpl(1,2)");
        REQUIRE(s_info_list.at(6) == "~AnimalImpl(1)");

        holder.set_child(Animal{5});

        REQUIRE(s_info_list.size() == 10u);
        REQUIRE(s_info_list.at(7) == "AnimalImpl(5)");
        REQUIRE(s_info_list.at(8) == "~DogImpl(3,4)");
        REQUIRE(s_info_list.at(9) == "~AnimalImpl(3)");

        holder = {};

        REQUIRE(s_info_list.size() == 12u);
        REQUIRE(s_info_list.at(10) == "~AnimalHolderImpl()");
        REQUIRE(s_info_list.at(11) == "~AnimalImpl(5)");
    }
}
