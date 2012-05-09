/*
 Copyright (C) 2012 MoSync AB

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License,
 version 2, as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 MA 02110-1301, USA.
 */

/**
 * @file PurchaseManager.cpp
 * @author Bogdan Iusco
 * @date 3 May 2012
 *
 * @brief The PurchaseManager manages purchase related events and dispatches
 * them to the target products.
 */

#include "PurchaseManager.h"
#include "PurchaseManagerListener.h"
#include "Purchase.h"

namespace Purchase
{
	/**
	 * Initialize the singleton variable to NULL.
	 */
	PurchaseManager* PurchaseManager::sInstance = NULL;

	/**
	*  Check if in app purchase is supported/enabled on a device.
	* @return One of the next result codes:
	* - MA_PURCHASE_RES_OK if purchase is supported/allowed on the device.
	* - MA_PURCHASE_RES_DISABLED if purchase is not allowed/enabled.
	*/
	int checkPurchaseSupported()
	{
		return maPurchaseSupported();
	}

	/**
	 * Constructor.
	 */
	PurchaseManager::PurchaseManager()
	{
		// Add me as a custom event listener.
		MAUtil::Environment::getEnvironment().addCustomEventListener(this);
	}

	/**
	 * Destructor.
	 */
	PurchaseManager::~PurchaseManager()
	{
		// Remove me as a custom event listener.
		MAUtil::Environment::getEnvironment().removeCustomEventListener(this);
		mListeners.clear();
		mSpecialProducts.clear();
	}

	/**
	 * Return the single instance of this class.
	 */
	PurchaseManager* PurchaseManager::getInstance()
	{
		if (NULL == PurchaseManager::sInstance)
		{
			PurchaseManager::sInstance = new PurchaseManager();
		}

		return sInstance;
	}

	/**
	 * Destroy the single instance of this class.
	 * Call this method only when the application will exit.
	 */
	void PurchaseManager::destroyInstance()
	{
		delete PurchaseManager::sInstance;
	}

	/**
	 * Platform: Android.
	 * @param developerPublicKey
	 */
	void PurchaseManager::setPublicKey(const MAUtil::String& developerPublicKey)
	{
		maPurchaseSetPublicKey(developerPublicKey.c_str());
	}

	/**
	 * Set the store URL used for verifying the receipt on iOS platform.
	 * Platform: iOS.
	 * @param url Recommended values: sAppStoreURL and sAppStoreSandboxURL.
	 */
	void PurchaseManager::setStoreURL(const MAUtil::String& url)
	{
		maPurchaseSetStoreURL(url.c_str());
	}

	/**
	 * Implementation of CustomEventListener interface.
	 * This method will get called whenever there is a
	 * widget event generated by the system.
	 * @param event The new received event.
	 */
	void PurchaseManager::customEvent(const MAEvent& event)
	{
		// Check if this is a purchase event.
		if (EVENT_TYPE_PURCHASE == event.type)
		{
			MAPurchaseEventData purchaseData = event.purchaseData;
			if (purchaseData.type == MA_PURCHASE_EVENT_RESTORE)
			{
				this->createRestoredProduct(purchaseData.productHandle);
			}
			else if (purchaseData.type == MA_PURCHASE_EVENT_REFUNDED)
			{
				this->createRefundedProduct(purchaseData.productHandle);
			}
			else
			{
				// Check if the purchase exists in the map.
				if (mPurchaseMap.end() != mPurchaseMap.find(purchaseData.productHandle))
				{
					// Get the product object that wraps the product handle.
					Purchase* purchase = mPurchaseMap[purchaseData.productHandle];

					// Call the purchase's event handling method.
					purchase->handlePurchaseEvent(event.purchaseData);
				}

			}
		}
	}

	/**
	 * Add a purchase to the map that holds purchases.
	 * The purchase will receive custom events.
	 * @param purchase The purchase that needs to be registered.
	 * The ownership of the purchase is not passed to this method.
	 */
	void PurchaseManager::registerPurchase(Purchase* purchase)
	{
		mPurchaseMap[purchase->getHandle()] = purchase;
	}

	/**
	 * Remove a purchase from the map that holds purchases.
	 * The purchase will not receive custom events.
	 * @param product The purchase that needs to be unregistered.
	 */
	void PurchaseManager::unregisterPurchase(Purchase* purchase)
	{
		mPurchaseMap.erase(purchase->getHandle());
	}

	/**
	 * Add an event listener for purchase.
	 * Listener will be notified about restored or refunded products.
	 * @param listener The listener that will receive notifications.
	 */
	void PurchaseManager::addPurchaseListener(PurchaseManagerListener* listener)
	{
		for (int i = 0; i < mListeners.size(); i++)
		{
			if (listener == mListeners[i])
			{
				return;
			}
		}

		mListeners.add(listener);

	}

	/**
	 * Remove the event listener for purchase.
	 * @param listener The listener that receives purchase notifications.
	 */
	void PurchaseManager::removePurchaseListener(PurchaseManagerListener* listener)
	{
		for (int i = 0; i < mListeners.size(); i++)
		{
			if (listener == mListeners[i])
			{
				mListeners.remove(i);
				break;
			}
		}
	}

	/**
	 * Create a product that has been restored and notifies the listeners.
	 * @param productHandle Handle to the product that has been restored.
	 */
	void PurchaseManager::createRestoredProduct(MAHandle productHandle)
	{
		Purchase* purchase = new Purchase(productHandle);
		for (int i = 0; i < mListeners.size(); i++)
		{
			mListeners[i]->purchaseRestored(*purchase);
		}
		mSpecialProducts.add(purchase);
	}

	/**
	 * Create a product that has been refunded and notifies the listeners.
	 * @param productHandle Handle to the product that has been refunded.
	 */
	void PurchaseManager::createRefundedProduct(MAHandle productHandle)
	{
		Purchase* purchase = new Purchase(productHandle);
		for (int i = 0; i < mListeners.size(); i++)
		{
			mListeners[i]->purchaseRefunded(*purchase);
		}
		mSpecialProducts.add(purchase);
	}
}
