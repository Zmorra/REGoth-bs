#include "GameWorld.hpp"
#include "internals/ConstructFromZEN.hpp"
#include <Components/BsCCharacterController.h>
#include <Debug/BsDebugDraw.h>
#include <Scene/BsSceneObject.h>
#include <components/Character.hpp>
#include <components/CharacterAI.hpp>
#include <components/Item.hpp>
#include <components/VisualCharacter.hpp>
#include <components/Waynet.hpp>
#include <components/Waypoint.hpp>
#include <exception/Throw.hpp>
#include <scripting/ScriptVMInterface.hpp>

namespace REGoth
{
  namespace World
  {
    class GameWorldInternal
    {
    public:
      GameWorldInternal()
      {
      }

      void loadEmpty()
      {
        mWorldName = "EmptyWorld";

        mSceneRoot = bs::SceneObject::create("SceneRoot");

        bs::HSceneObject waynetSO = bs::SceneObject::create("Waynet");
        waynetSO->setParent(mSceneRoot);
        waynetSO->addComponent<Waynet>();

        findWaynet();
      }

      bool loadFromZEN(const bs::String& zenFile)
      {
        assignWorldNameFromZEN(zenFile);

        mSceneRoot = constructFromZEN(zenFile);

        if (!mSceneRoot) return false;

        findWaynet();

        return true;
      }

      void assignWorldNameFromZEN(const bs::String& zenFile)
      {
        mWorldName = zenFile.substr(0, zenFile.find_first_of('.'));
      }

      void findWaynet()
      {
        bs::HSceneObject waynetSO = gWorld().sceneRoot()->findChild("Waynet");

        if (!waynetSO)
        {
          REGOTH_THROW(InvalidStateException, "No waynet found in this world?");
        }

        HWaynet waynet = waynetSO->getComponent<Waynet>();

        if (!waynet)
        {
          REGOTH_THROW(InvalidStateException, "Waynet does not have Waynet component?");
        }

        mWaynet = waynet;
      }

      void createHero()
      {
        HCharacter hero = insertCharacter("PC_HERO", mWaynet->allWaypoints()[0]->SO()->getName());
        hero->useAsHero();
      }

      HItem insertItem(const bs::String& instance, const bs::Transform& transform)
      {
        bs::HSceneObject itemSO = bs::SceneObject::create(instance);
        itemSO->setPosition(transform.pos());
        itemSO->setRotation(transform.rot());

        return itemSO->addComponent<Item>(instance);
      }

      HItem insertItem(const bs::String& instance, const bs::String& spawnPoint)
      {
        bs::HSceneObject spawnPointSO = mSceneRoot->findChild(spawnPoint);
        bs::Transform transform;

        if (spawnPointSO)
        {
          transform = spawnPointSO->getTransform();
        }
        else
        {
          // FIXME: What to do on invalid spawnpoints?
          // REGOTH_THROW(InvalidParametersException,
          //     bs::StringUtil::format("Spawnpoint {0} for item of instance {1} does not exist!",
          //                            spawnPoint, instance));
        }

        return insertItem(instance, transform);
      }

      HCharacter insertCharacter(const bs::String& instance, const bs::Transform& transform)
      {
        bs::HSceneObject characterSO = bs::SceneObject::create(instance);
        characterSO->setParent(mSceneRoot);

        characterSO->setPosition(transform.pos());
        characterSO->setRotation(transform.rot());

        characterSO->addComponent<VisualCharacter>();
        characterSO->addComponent<bs::CCharacterController>();
        characterSO->addComponent<CharacterAI>();

        return characterSO->addComponent<Character>(instance);
      }

      HCharacter insertCharacter(const bs::String& instance, const bs::String& spawnPoint)
      {
        bs::HSceneObject spawnPointSO = mSceneRoot->findChild(spawnPoint);
        bs::Transform transform;

        if (spawnPointSO)
        {
          transform = spawnPointSO->getTransform();
        }
        else
        {
          // FIXME: What to do on invalid spawnpoints?
          // REGOTH_THROW(
          //     InvalidParametersException,
          //     bs::StringUtil::format("Spawnpoint '{0}' for character of instance {1} does not
          //     exist!",
          //                            spawnPoint, instance));
        }

        transform.move(
            bs::Vector3(0, 0.5f, 0));  // FIXME: Can we move the center to the feet somehow instead?

        return insertCharacter(instance, transform);
      }

      bs::String mWorldName;
      bs::HSceneObject mSceneRoot;
      HWaynet mWaynet;
    };

    bool GameWorld::_loadZen(const bs::String& zenFile, Init init)
    {
      mInternal = bs::bs_shared_ptr_new<GameWorldInternal>();

      bool hasLoaded = mInternal->loadFromZEN(zenFile);

      if (!hasLoaded) return false;

      if (init != Init::NoInitScripts)
      {
        mInternal->createHero();

        gGameScript().initializeWorld(worldName());
      }

      return true;
    }

    void GameWorld::_loadEmpty()
    {
      mInternal = bs::bs_shared_ptr_new<GameWorldInternal>();
      mInternal->loadEmpty();
    }

    const bs::String& GameWorld::worldName()
    {
      return mInternal->mWorldName;
    }

    bs::HSceneObject GameWorld::sceneRoot()
    {
      return mInternal->mSceneRoot;
    }

    HWaynet GameWorld::waynet()
    {
      return mInternal->mWaynet;
    }

    HItem GameWorld::insertItem(const bs::String& instance, const bs::Transform& transform)
    {
      return mInternal->insertItem(instance, transform);
    }

    HItem GameWorld::insertItem(const bs::String& instance, const bs::String& spawnPoint)
    {
      return mInternal->insertItem(instance, spawnPoint);
    }

    HCharacter GameWorld::insertCharacter(const bs::String& instance, const bs::String& waypoint)
    {
      return mInternal->insertCharacter(instance, waypoint);
    }

    static bs::SPtr<World::GameWorld> s_World;

    bool loadWorldFromZEN(const bs::String& zenFile, GameWorld::Init init)
    {
      s_World = bs::bs_shared_ptr_new<World::GameWorld>();
      s_World->_loadZen(zenFile, init);
      return true;
    }

    void loadWorldEmpty()
    {
      s_World = bs::bs_shared_ptr_new<World::GameWorld>();
      s_World->_loadEmpty();
    }

  }  // namespace World

  World::GameWorld& gWorld()
  {
    if (!World::s_World)
    {
      REGOTH_THROW(InvalidStateException, "No world has been loaded yet!");
    }

    return *World::s_World;
  }
}  // namespace REGoth
